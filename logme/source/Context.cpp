#include <Logme/Channel.h>
#include <Logme/Context.h>
#include <Logme/Time/datetime.h>
#include <Logme/Utils.h>
#include "StringHelpers.h"

#include <cassert>
#include <string>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef _WIN32
#pragma warning(disable : 26812)
#endif

#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

using namespace Logme;


namespace
{
  const char* GetStructuredLevelName(Level level)
  {
    switch (level)
    {
    case Level::LEVEL_DEBUG: return "debug";
    case Level::LEVEL_INFO: return "info";
    case Level::LEVEL_WARN: return "warn";
    case Level::LEVEL_ERROR: return "error";
    case Level::LEVEL_CRITICAL: return "critical";
    default: return "unknown";
    }
  }

  size_t TrimRight(const char* text, size_t len)
  {
    while (len && (text[len - 1] == ' ' || text[len - 1] == '\t' || text[len - 1] == '\r' || text[len - 1] == '\n'))
      --len;

    return len;
  }

  void AppendJsonEscaped(std::string& output, const char* text, size_t len)
  {
    output.push_back('"');

    for (size_t i = 0; i < len; ++i)
    {
      unsigned char c = (unsigned char)text[i];
      switch (c)
      {
      case '"': output += "\\\""; break;
      case '\\': output += "\\\\"; break;
      case '\b': output += "\\b"; break;
      case '\f': output += "\\f"; break;
      case '\n': output += "\\n"; break;
      case '\r': output += "\\r"; break;
      case '\t': output += "\\t"; break;
      default:
        if (c < 0x20)
        {
          char tmp[7];
          snprintf(tmp, sizeof(tmp), "\\u%04X", (unsigned)c);
          output += tmp;
        }
        else
        {
          output.push_back((char)c);
        }
      }
    }

    output.push_back('"');
  }

  void AppendJsonEscaped(std::string& output, const char* text)
  {
    AppendJsonEscaped(output, text ? text : "", text ? strlen(text) : 0);
  }

  void AppendXmlEscaped(std::string& output, const char* text, size_t len)
  {
    for (size_t i = 0; i < len; ++i)
    {
      switch (text[i])
      {
      case '&': output += "&amp;"; break;
      case '<': output += "&lt;"; break;
      case '>': output += "&gt;"; break;
      case '\"': output += "&quot;"; break;
      case '\'': output += "&apos;"; break;
      default: output.push_back(text[i]);
      }
    }
  }

  void AppendXmlEscaped(std::string& output, const char* text)
  {
    AppendXmlEscaped(output, text ? text : "", text ? strlen(text) : 0);
  }

  void AppendJsonFieldName(std::string& output, bool& first, OutputField field)
  {
    if (!first)
      output.push_back(',');

    first = false;
    AppendJsonEscaped(output, GetOutputFieldName(field));
    output.push_back(':');
  }

  void AppendJsonStringField(std::string& output, bool& first, OutputField field, const char* value)
  {
    AppendJsonFieldName(output, first, field);
    AppendJsonEscaped(output, value);
  }

  void AppendJsonStringField(std::string& output, bool& first, OutputField field, const char* value, size_t len)
  {
    AppendJsonFieldName(output, first, field);
    AppendJsonEscaped(output, value, len);
  }

  void AppendJsonUInt64Field(std::string& output, bool& first, OutputField field, uint64_t value)
  {
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "%llu", (unsigned long long)value);
    AppendJsonFieldName(output, first, field);
    output += tmp;
  }

  void AppendXmlElement(std::string& output, OutputField field, const char* value)
  {
    const char* name = GetOutputFieldName(field);
    output.push_back('<');
    output += name;
    output.push_back('>');
    AppendXmlEscaped(output, value);
    output += "</";
    output += name;
    output.push_back('>');
  }

  void AppendXmlElement(std::string& output, OutputField field, const char* value, size_t len)
  {
    const char* name = GetOutputFieldName(field);
    output.push_back('<');
    output += name;
    output.push_back('>');
    AppendXmlEscaped(output, value, len);
    output += "</";
    output += name;
    output.push_back('>');
  }

  void AppendXmlUInt64Element(std::string& output, OutputField field, uint64_t value)
  {
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "%llu", (unsigned long long)value);
    AppendXmlElement(output, field, tmp);
  }
}

#ifndef _WIN32
#define strtok_s strtok_r
#define _stricmp strcasecmp
#endif

Context::Context(ContextCache& cache, Level level, const ID* ch, const SID* sid)
  : Cache(cache)
  , ChannelStg{}
  , Channel(ch)
  , Subsystem(sid ? *sid : SUBSID)
  , ErrorLevel(level)
  , Method(nullptr)
  , File(nullptr)
  , Line(0)
  , Ch(nullptr)
  , AppendProc(nullptr)
  , AppendContext(nullptr)
  , Ovr(nullptr)
  , Signature(0)
  , ExtBufferSize(0)
  , TempBuffer(nullptr)
  , TempBufferSize(0)
  , TempBufferCapacity(0)
{
  InitContext();
}

Context::Context(
  ContextCache& cache
  , Level level
  , const ID* chdef
  , const SID* siddef
  , const char* method
  , const char* file
  , int line
  , const Context::Params& params
)
  : Cache(cache)
  , ChannelStg{}
  , Channel(&params.Channel)
  , Subsystem(params.Subsystem)
  , ErrorLevel(level)
  , Method(method)
  , File(file)
  , Line(line)
  , Ch(nullptr)
  , AppendProc(nullptr)
  , AppendContext(nullptr)
  , Ovr(nullptr)
  , Signature(0)
  , ExtBufferSize(0)
  , TempBuffer(nullptr)
  , TempBufferSize(0)
  , TempBufferCapacity(0)
{
  if (Channel->Name == nullptr)
    Channel = chdef;

  if (Subsystem.Name == 0)
    Subsystem = *siddef;

  InitContext();
}

void Context::InitContext()
{
  *Timestamp = '\0';
  *ThreadProcessID = '\0';

  *Buffer = '\0';
  LastData = nullptr;
  LastLen = 0;
  TempBuffer = nullptr;
  TempBufferSize = 0;
  TempBufferCapacity = 0;
  Applied.None = true;
}

void Context::SetText(const char* text)
{
  assert(TempBuffer == nullptr);
  assert(TempBufferSize == 0);
  assert(TempBufferCapacity == 0);

  if (text == nullptr)
    text = "";

  size_t len = strlen(text);
  size_t capacity = len + 16;

  Storage.reserve(capacity);
  capacity = Storage.capacity();
  Storage.resize(capacity);

  if (len)
    memcpy(Storage.data(), text, len);

  Storage.data()[len] = '\0';

  TempBuffer = Storage.data();
  TempBufferSize = len;
  TempBufferCapacity = capacity;
}

void Context::SetBuffer(const char* buffer, size_t size, size_t capacity)
{
  assert(TempBuffer == nullptr);
  assert(TempBufferSize == 0);
  assert(TempBufferCapacity == 0);

  TempBuffer = buffer;
  TempBufferSize = size;
  TempBufferCapacity = capacity;
}

const char* Context::GetText() const
{
  return TempBuffer;
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

void Context::InitThreadProcessID(const ChannelPtr& ch, OutputFlags flags)
{
  if ((flags.ProcessID || flags.ThreadID) && *ThreadProcessID == '\0')
  {
#ifdef _WIN32
    auto thread = GetCurrentThreadId();
    static auto process = GetCurrentProcessId();
#else
    auto thread = GetCurrentThreadId();
    static uint64_t pid = (uint64_t)GetCurrentProcessId();
    auto process = pid;
#endif

    char* p = ThreadProcessID;
    size_t remaining = sizeof(ThreadProcessID) - 4; // '[', ']', ' ' and \0
    *p++ = '[';

    if (flags.ProcessID)
    {
      int c = sprintf(p, LOGME_FMT_U64_HEX_UPPER, (uint64_t)process);
      p += c;
      remaining -= c;
    }

    if (flags.ThreadID)
    {
      *p++ = ':';
      --remaining;

      do
      {
        Channel::ThreadNameInfo info;
        std::optional<std::string> trans;
        auto name = ch->GetThreadName(thread, info, &trans, true);

        if (flags.ThreadTransition)
        {
          if (name == nullptr && trans.has_value() == false)
          {
            ChannelPtr link = ch->GetLinkPtr();

            if (link)
              name = link->GetThreadName(thread, info, &trans, true);
          }

          if (trans.has_value())
          {
            const std::string& t = trans.value();
            if (t.size() >= remaining)
            {
              memcpy(p, t.c_str(), remaining);
              p += remaining;
              *p = '\0';
              remaining = 0;
            }
            else
            {
              strcpy(p, t.c_str());
              p += t.size();
              remaining -= t.size();
            }

            if (remaining >= 4)
            {
              strcpy(p, " -> ");
              p += 4;
              remaining -= 4;
            }
          }
        }

        if (name)
        {
          size_t n = strlen(name);
          if (n > remaining)
          {
            memcpy(p, name, remaining);
            p += remaining;
            *p = '\0';
          }
          else
          {
            strcpy(p, name);
            p += n;
          }
        }
        else if (remaining > 0)
        {
          int c = snprintf(p, remaining, LOGME_FMT_U64_HEX_UPPER, (uint64_t)thread); 
          if (c < 0)
              c = 0;

          if ((size_t)c >= remaining)
            p += remaining - 1;
          else
            p += c;
        }
      
      } while (false);
    }

    *p++ = ']';
    *p++ = ' ';
    *p = '\0';
  }
  else if (!(flags.ProcessID || flags.ThreadID))
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


const char* Context::ApplyJson(const ChannelPtr& ch, OutputFlags flags, int& nc)
{
  if (Applied.Value == flags.Value)
  {
    nc = LastLen;
    return LastData;
  }

  if (flags.Timestamp != TIME_FORMAT_NONE && *Timestamp == '\0')
    InitTimestamp((TimeFormat)flags.Timestamp);

  if (flags.Method && Method && !flags.ProcPrint)
  {
    Method = ch->ShortenerRun(Method, MethodShortener);

    if (Ovr != nullptr)
      Method = ch->ShortenerRun(Method, MethodShortener, *Ovr);
  }

  const char* appendText = nullptr;
  if (flags.Duration && !flags.ProcPrintIn && AppendProc)
    appendText = AppendProc(*this);

  std::string output;
  output.reserve(TempBufferSize + 256);
  output.push_back('{');

  bool first = true;
  if (flags.Timestamp != TIME_FORMAT_NONE)
  {
    size_t len = TrimRight(Timestamp, strlen(Timestamp));
    AppendJsonStringField(output, first, OUTPUT_FIELD_TIMESTAMP, Timestamp, len);
  }

  if (flags.Signature)
    AppendJsonStringField(output, first, OUTPUT_FIELD_LEVEL, GetStructuredLevelName(ErrorLevel));

  if (flags.ProcessID)
    AppendJsonUInt64Field(output, first, OUTPUT_FIELD_PROCESS_ID, (uint64_t)GetCurrentProcessId());

  if (flags.ThreadID)
    AppendJsonUInt64Field(output, first, OUTPUT_FIELD_THREAD_ID, (uint64_t)GetCurrentThreadId());

  if (flags.Channel)
    AppendJsonStringField(output, first, OUTPUT_FIELD_CHANNEL, Channel->Name ? Channel->Name : "");

  if (flags.Subsystem && Subsystem.Name)
  {
    char subsystem[9];
    memcpy(subsystem, (const char*)&Subsystem.Name, 8);
    subsystem[8] = '\0';
    AppendJsonStringField(output, first, OUTPUT_FIELD_SUBSYSTEM, subsystem, TrimRight(subsystem, strlen(subsystem)));
  }

  if (flags.Location)
  {
    AppendJsonStringField(
      output
      , first
      , OUTPUT_FIELD_FILE
      , flags.Location == DETALITY_SHORT ? File.GetShortName() : File.FullName
    );
    AppendJsonUInt64Field(output, first, OUTPUT_FIELD_LINE, (uint64_t)Line);
  }

  if (flags.Method && Method && !flags.ProcPrint)
    AppendJsonStringField(output, first, OUTPUT_FIELD_METHOD, Method);

  AppendJsonStringField(output, first, OUTPUT_FIELD_MESSAGE, TempBuffer, TempBufferSize);

  if (appendText)
    AppendJsonStringField(output, first, OUTPUT_FIELD_DURATION, appendText);

  output.push_back('}');

  if (flags.Eol)
    output.push_back('\n');

  size_t required = output.size() + 1;
  char* buffer = Buffer;
  if (required > sizeof(Buffer))
  {
    if (ExtBufferSize < required)
    {
      ExtBuffer = std::make_unique<char[]>(required);
      ExtBufferSize = required;
    }

    buffer = ExtBuffer.get();
  }

  memcpy(buffer, output.data(), output.size());
  buffer[output.size()] = '\0';

  Applied.Value = flags.Value;
  LastData = buffer;
  LastLen = (int)output.size();
  nc = LastLen;
  return LastData;
}

const char* Context::ApplyXml(const ChannelPtr& ch, OutputFlags flags, int& nc)
{
  if (Applied.Value == flags.Value)
  {
    nc = LastLen;
    return LastData;
  }

  if (flags.Timestamp != TIME_FORMAT_NONE && *Timestamp == '\0')
    InitTimestamp((TimeFormat)flags.Timestamp);

  if (flags.Method && Method && !flags.ProcPrint)
  {
    Method = ch->ShortenerRun(Method, MethodShortener);

    if (Ovr != nullptr)
      Method = ch->ShortenerRun(Method, MethodShortener, *Ovr);
  }

  const char* appendText = nullptr;
  if (flags.Duration && !flags.ProcPrintIn && AppendProc)
    appendText = AppendProc(*this);

  std::string output;
  output.reserve(TempBufferSize + 256);
  output += "<event>";

  if (flags.Timestamp != TIME_FORMAT_NONE)
  {
    size_t len = TrimRight(Timestamp, strlen(Timestamp));
    AppendXmlElement(output, OUTPUT_FIELD_TIMESTAMP, Timestamp, len);
  }

  if (flags.Signature)
    AppendXmlElement(output, OUTPUT_FIELD_LEVEL, GetStructuredLevelName(ErrorLevel));

  if (flags.ProcessID)
    AppendXmlUInt64Element(output, OUTPUT_FIELD_PROCESS_ID, (uint64_t)GetCurrentProcessId());

  if (flags.ThreadID)
    AppendXmlUInt64Element(output, OUTPUT_FIELD_THREAD_ID, (uint64_t)GetCurrentThreadId());

  if (flags.Channel)
    AppendXmlElement(output, OUTPUT_FIELD_CHANNEL, Channel->Name ? Channel->Name : "");

  if (flags.Subsystem && Subsystem.Name)
  {
    char subsystem[9];
    memcpy(subsystem, (const char*)&Subsystem.Name, 8);
    subsystem[8] = '\0';
    AppendXmlElement(output, OUTPUT_FIELD_SUBSYSTEM, subsystem, TrimRight(subsystem, strlen(subsystem)));
  }

  if (flags.Location)
  {
    AppendXmlElement(
      output
      , OUTPUT_FIELD_FILE
      , flags.Location == DETALITY_SHORT ? File.GetShortName() : File.FullName
    );
    AppendXmlUInt64Element(output, OUTPUT_FIELD_LINE, (uint64_t)Line);
  }

  if (flags.Method && Method && !flags.ProcPrint)
    AppendXmlElement(output, OUTPUT_FIELD_METHOD, Method);

  AppendXmlElement(output, OUTPUT_FIELD_MESSAGE, TempBuffer, TempBufferSize);

  if (appendText)
    AppendXmlElement(output, OUTPUT_FIELD_DURATION, appendText);

  output += "</event>";

  if (flags.Eol)
    output.push_back('\n');

  size_t required = output.size() + 1;
  char* buffer = Buffer;
  if (required > sizeof(Buffer))
  {
    if (ExtBufferSize < required)
    {
      ExtBuffer = std::make_unique<char[]>(required);
      ExtBufferSize = required;
    }

    buffer = ExtBuffer.get();
  }

  memcpy(buffer, output.data(), output.size());
  buffer[output.size()] = '\0';

  Applied.Value = flags.Value;
  LastData = buffer;
  LastLen = (int)output.size();
  nc = LastLen;
  return LastData;
}

const char* Context::Apply(const ChannelPtr& ch, OutputFlags flags, int& nc)
{
  assert(TempBuffer != nullptr);

  const char* text = TempBuffer;
  if (Ovr)
  {
    flags.Value |= Ovr->Add.Value;
    flags.Value &= ~Ovr->Remove.Value;
  }

  // Copy control flags
  flags.ProcPrint = Applied.ProcPrint;
  flags.ProcPrintIn = Applied.ProcPrintIn;

  if (flags.Format == OUTPUT_JSON)
    return ApplyJson(ch, flags, nc);

  if (flags.Format == OUTPUT_XML)
    return ApplyXml(ch, flags, nc);

  if (Applied.Value == flags.Value)
  {
    nc = LastLen;
    return LastData;
  }

  *Buffer = '\0';

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
  
  int nSubsystem = 0;
  char subsystem[128]{};
  if (flags.Subsystem && Subsystem.Name)
  {
    sprintf(subsystem, "#%.8s ", (const char*)&Subsystem.Name);
    nSubsystem = (int)strlen(subsystem);
  }

  int nFile = 0;
  char line[32] = {0};
  if (flags.Location)
  {
    sprintf_s(line, sizeof(line), "(%i): ", Line);

    if (flags.Location == DETALITY_SHORT)
      nFile = int(strlen(File.GetShortName()) + strlen(line));
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

    if (Ovr != nullptr)
      Method = ch->ShortenerRun(Method, MethodShortener, *Ovr);

    nMethod = (int)strlen(Method) + 4; // "Method(): "
  }

  int nEol = 0;
  if (flags.Eol)
    nEol = 1;

  int nLine = (int)TempBufferSize;

  int nAppend = 0;
  const char* appendText = "";
  if (flags.Duration && !flags.ProcPrintIn && AppendProc)
  {
    appendText = AppendProc(*this);

    if (appendText)
      nAppend = (int)strlen(appendText);
  }

  bool canReuseTempBuffer =
    TempBuffer != nullptr
    && TempBufferSize == (size_t)nLine
    && nTimestamp == 0
    && nSignature == 0
    && nID == 0
    && nChannel == 0
    && nSubsystem == 0
    && nFile == 0
    && nError == 0
    && nMethod == 0
  ;

  if (canReuseTempBuffer)
  {
    size_t required = (size_t)nLine + (size_t)nAppend + (size_t)nEol + 1;
    if (required <= TempBufferCapacity)
    {
      char* p = const_cast<char*>(text) + nLine;

      if (nAppend)
      {
        strcpy_s(p, TempBufferCapacity - (size_t)(p - TempBuffer), appendText);
        p += nAppend;
      }

      if (nEol)
      {
        *p++ = '\n';
        *p = '\0';
      }

      LastData = text;
      LastLen = nLine + nAppend + nEol;
      Applied.Value = flags.Value;
      
      nc = LastLen;
      return LastData;
    }
  }

  char* buffer = Buffer;
  int n = nTimestamp + nSignature + nID + nChannel + nSubsystem + nFile + nError + nMethod + nLine + nAppend + nEol + 1;
  if (n > (int)sizeof(Buffer))
  {
    if (ExtBufferSize < size_t(n))
    {
      ExtBuffer = std::make_unique<char[]>(size_t(n));
      ExtBufferSize = size_t(n);
    }

    buffer = ExtBuffer.get();
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

  if (nSubsystem)
  {
    strcpy(p, subsystem);
    p += nSubsystem;
  }

  if (nFile)
  {
    sprintf_s(p
      , n - (p - buffer)
      , "%s%s"
      , flags.Location == DETALITY_SHORT ? File.GetShortName() : File.FullName
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
    strcpy(p, Method);    
    strcpy(p + nMethod - 4, "(): ");
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

