#include <Logme/Channel.h>
#include <Logme/Context.h>
#include <Logme/Time/datetime.h>
#include "StringHelpers.h"

#include <cassert>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#pragma warning(disable : 26812)

using namespace Logme;

#ifndef _WIN32
#define strtok_s strtok_r
#define _stricmp strcasecmp
#endif

Context::Context(Level level, const ID* ch)
  : ChannelStg{}
  , Channel(ch)
  , ErrorLevel(level)
  , Method(nullptr)
  , File(nullptr)
  , Line(0)
  , AppendProc(nullptr)
  , AppendContext(nullptr)
  , Signature(0)
{
  InitContext();
}

Context::Context(
  Level level
  , const ID* chdef
  , const char* method
  , const char* file
  , int line
  , const Context::Params& params
)
  : ChannelStg{}
  , Channel(&params.Channel)
  , ErrorLevel(level)
  , Method(method)
  , File(file)
  , Line(line)
  , AppendProc(nullptr)
  , AppendContext(nullptr)
  , Signature(0)
{
  if (Channel->Name == nullptr)
    Channel = chdef;

  InitContext();
}

void Context::InitContext()
{
  *Timestamp = '\0';
  *ThreadProcessID = '\0';

  *Buffer = '\0';
  LastData = nullptr;
  LastLen = 0;
  Applied.None = true;
}

void Context::CreateTZD(char* tzd)
{
  time_t now = time(0);

#ifdef _WIN32
  struct tm local;
  localtime_s(&local, &now);
#else
  struct tm local = *localtime(&now);
#endif

#ifdef _WIN32
  struct tm utc;
  gmtime_s(&utc, &now);
#else
  struct tm utc = *gmtime(&now);
#endif

  time_t t1 = mktime(&local);
  time_t t2 = mktime(&utc);
  int32_t d = int32_t(t1 - t2);

  time_t m = abs(d) % 60;
  time_t h = (d / (60 * 60)) % 60;

  snprintf(tzd, size_t(TZD_BUFFER_SIZE) - 1, "%+.02i:%02i ", (int)h, (int)m);
} 

void Context::InitTimestamp(TimeFormat tf)
{
  auto n = GetTimeInMillisec();

  switch (tf)
  {
  case TimeFormat::TIME_FORMAT_LOCAL:
  case TimeFormat::TIME_FORMAT_TZ:
  {
    static std::mutex Lock;
    static DateTime LastStamp{};
    static unsigned int LastStampTicks = 0;
    static char timestamp[TIMESTAMP_BUFFER_SIZE]{};

    std::lock_guard lock(Lock);

    if (n != LastStampTicks)
    {
      DateTime stamp = DateTime::Now();

      LastStamp = stamp;
      LastStampTicks = n;

      snprintf(
        timestamp
        , size_t(TIMESTAMP_BUFFER_SIZE) - 1
        , "%04i-%02i-%02i %02i:%02i:%02i:%03i "
        , stamp.GetYear()
        , stamp.GetMonth()
        , stamp.GetDay()
        , stamp.GetHour()
        , stamp.GetMinute()
        , stamp.GetSecond()
        , stamp.GetMillisecond()
      );

      timestamp[TIMESTAMP_BUFFER_SIZE - 1] = '\0';
    }

    memcpy(Timestamp, timestamp, TIMESTAMP_BUFFER_SIZE);
    break;
  }

  case TimeFormat::TIME_FORMAT_UTC:
  {
    static std::mutex Lock;
    static DateTime LastStamp;
    static unsigned int LastStampTicks{};
    static char timestamp[TIMESTAMP_BUFFER_SIZE]{};

    std::lock_guard lock(Lock);

    if (n != LastStampTicks)
    {
      DateTime stamp = DateTime::NowUtc();

      LastStamp = stamp;
      LastStampTicks = n;

      snprintf(
        timestamp
        , size_t(TIMESTAMP_BUFFER_SIZE) - 1
        , "%04i-%02i-%02i %02i:%02i:%02i:%03i "
        , stamp.GetYear()
        , stamp.GetMonth()
        , stamp.GetDay()
        , stamp.GetHour()
        , stamp.GetMinute()
        , stamp.GetSecond()
        , stamp.GetMillisecond()
      );

      timestamp[TIMESTAMP_BUFFER_SIZE - 1] = '\0';
    }

    memcpy(Timestamp, timestamp, TIMESTAMP_BUFFER_SIZE);
    break;
  }

  default:
    assert(!"unexpected TimeFormat");
  }

  if (tf == TimeFormat::TIME_FORMAT_TZ)
  {
    char tzd[TIMESTAMP_BUFFER_SIZE];
    CreateTZD(tzd);

    Timestamp[TZD_T_POS] = 'T';
    strcpy_s(Timestamp + TZD_E_POS, size_t(TIMESTAMP_BUFFER_SIZE) - TZD_E_POS, tzd);
  }
}

void Context::InitThreadProcessID(ChannelPtr ch, OutputFlags flags)
{
  if ((flags.ProcessID || flags.ThreadID) && *ThreadProcessID == '\0')
  {
#ifdef _WIN32
    auto thread = GetCurrentThreadId();
    static auto process = GetCurrentProcessId();
#else
    auto thread = getid();
    static auto process = getpid();
#endif

    char* p = ThreadProcessID;
    *p++ = '[';

    if (flags.ProcessID)
      p += sprintf(p, "%X", process);

    if (flags.ThreadID)
    {
      *p++ = ':';

      auto name = ch->GetThreadName(thread);
      if (name)
      {
        strcpy(p, name);
        p += strlen(name);
      }
      else
        p += sprintf(p, "%llX", (uint64_t)thread);
    }

    *p++ = ']';
    *p++ = ' ';
    *p = '\0';
  }
  else
    *ThreadProcessID = '\0';
}

void Context::InitSignature()
{
  switch (ErrorLevel)
  {
    case Level::LEVEL_DEBUG: Signature = 'D'; break;
    case Level::LEVEL_WARN: Signature = 'W'; break;
    case Level::LEVEL_ERROR: Signature = 'E'; break;
    case Level::LEVEL_CRITICAL: Signature = 'C'; break;
    default: Signature = ' ';
  }
}

const char* Context::Apply(ChannelPtr ch, OutputFlags flags, const char* text, int& nc)
{
  flags.Value |= Ovr.Add.Value;
  flags.Value &= ~Ovr.Remove.Value;

  // Copy control flags
  flags.ProcPrint = Applied.ProcPrint;
  flags.ProcPrintIn = Applied.ProcPrintIn;

  if (Applied.Value == flags.Value)
  {
    nc = LastLen;
    return LastData;
  }

  *Buffer = '\0';
  int offset = 0;

  int nTimestamp = 0;
  if (flags.Timestamp != TIME_FORMAT_NONE)
  {
    if (*Timestamp == '\0')
      InitTimestamp((TimeFormat)flags.Timestamp);

    nTimestamp = (int)strlen(Timestamp);
  }

  int nSignature = 0;
  if (flags.Signature)
  {
    if (Signature == '\0')
      InitSignature();

    nSignature = 2;
  }

  int nID = 0;
  if (flags.ProcessID || flags.ThreadID)
  {
    if (*ThreadProcessID == '\0')
      InitThreadProcessID(ch, flags);

    nID = (int)strlen(ThreadProcessID);
  }

  int nChannel = 0;
  if (flags.Channel)
  {
    int c = Channel->Name ? (int)strlen(Channel->Name) : 0;
    nChannel = c + 3; // "{c} "
  }

  int nFile = 0;
  char line[32] = {0};
  if (flags.Location)
  {
    sprintf_s(line, "(%i): ", Line);

    if (flags.Location == DETALITY_SHORT)
      nFile = int(strlen(File.ShortName) + strlen(line));
    else
      nFile = int(strlen(File.FullName) + strlen(line));
  }

  int nError = 0;
  const char* prefix = nullptr;
  if (flags.ErrorPrefix && ErrorLevel >= Level::LEVEL_ERROR)
  {
    if (ErrorLevel == Level::LEVEL_ERROR)
      prefix = "Error: ";
    else
      prefix = "Critical: ";

    nError = (int)strlen(prefix);
  }

  int nMethod = 0;
  if (flags.Method && Method && !flags.ProcPrint)
  {
    Method = ch->ShortenerRun(Method, MethodShortener);
    nMethod = (int)strlen(Method) + 4; // "Method(): "
  }

  int nEol = 0;
  if (flags.Eol)
    nEol = 1;

  int nLine = (int)strlen(text);

  int nAppend = 0;
  const char* appendText = "";
  if (flags.Duration && !flags.ProcPrintIn && AppendProc)
  {
    appendText = AppendProc(*this);

    if (appendText)
      nAppend = (int)strlen(appendText);
  }

  char* buffer = Buffer;
  int n = nTimestamp + nSignature + nID + nChannel + nFile + nError + nMethod + nLine + nAppend + nEol + 1;
  if (n > sizeof(Buffer))
  {
    if (ExtBuffer.size() < size_t(n))
      ExtBuffer.resize(n);

    buffer = &ExtBuffer[0];
  }

  char* p = buffer;
  if (nTimestamp)
  {
    strcpy_s(p, n - (p - buffer), Timestamp);
    p += nTimestamp;
  }

  if (nSignature)
  {
    *p++ = Signature;
    *p++ = ' ';
    *p = '\0';
  }

  if (nID)
  {
    strcpy_s(p, n - (p - buffer), ThreadProcessID);
    p += nID;
  }

  if (nChannel)
  {
    sprintf_s(p
      , n - (p - buffer)
      , "{%s} "
      , Channel->Name ? Channel->Name : ""
    );
    p += nChannel;
  }

  if (nFile)
  {
    sprintf_s(p
      , n - (p - buffer)
      , "%s%s"
      , flags.Location == DETALITY_SHORT ? File.ShortName : File.FullName
      , line
    );
    p += nFile;
  }

  if (nError)
  {
    strcpy_s(p, n - (p - buffer), prefix);
    p += nError;
  }

  if (nMethod)
  {
    sprintf_s(p, n - (p - buffer), "%s(): ", Method);
    p += nMethod;
  }

  strcpy_s(p, n - (p - buffer), text);
  p += nLine;

  if (nAppend)
  {
    strcpy_s(p, n - (p - buffer), appendText);
    p += nAppend;
  }

  if (nEol)
  {
    *p++ = '\n';
    *p++ = '\0';
    assert(n - (p - buffer) >= 0);
  }

  Applied.Value = flags.Value;
  LastData = buffer;
  LastLen = n - 1;

  nc = LastLen;
  return buffer;
}

