#include <chrono>
#include <cstdio>
#include <cstring>

#include <Logme/AnsiColorEscape.h>
#include <Logme/Backend/ConsoleBackend.h>
#include <Logme/Channel.h>
#include <Logme/Colorizer.h>
#include <Logme/Console/ConsoleManagerFactory.h>
#include <Logme/File/exe_path.h>
#include <Logme/Logger.h>

using namespace Logme;

ConsoleBackend::ConsoleBackend(ChannelPtr owner)
  : Backend(owner, TYPE_ID)
  , Async(false)
  , Registered(false)
  , ShutdownFlag(false)
  , ShutdownCalled(owner == nullptr)
  , QueueRecordLimit(QUEUE_RECORD_LIMIT)
  , QueueByteLimit(QUEUE_BYTE_LIMIT)
  , OverflowPolicy(ConsoleOverflowPolicy::BLOCK)
{
  if (owner)
  {
    auto logger = owner->GetOwner();
    if (logger->GetEnableVTMode())
      Colorizer::EnableVTMode();
  }
}

ConsoleBackend::~ConsoleBackend()
{
  Freeze();
}

void ConsoleBackend::SetAsync(bool async)
{
  Async = async;
}

bool ConsoleBackend::GetAsync() const
{
  return Async;
}

void ConsoleBackend::SetQueueLimits(size_t maxRecords, size_t maxBytes)
{
  QueueRecordLimit = maxRecords;
  QueueByteLimit = maxBytes;

  if (Registered.load(std::memory_order_relaxed))
    GetFactory().SetLimits(maxRecords, maxBytes);
}

void ConsoleBackend::SetOverflowPolicy(ConsoleOverflowPolicy policy)
{
  OverflowPolicy = policy;

  if (Registered.load(std::memory_order_relaxed))
    GetFactory().SetOverflowPolicy(policy);
}

void ConsoleBackend::Freeze()
{
  ShutdownFlag.store(true, std::memory_order_relaxed);
  Backend::Freeze();

  if (Registered.exchange(false, std::memory_order_relaxed))
    GetFactory().Remove(this);
  else
    GetFactory().Flush();
}

bool ConsoleBackend::IsIdle() const
{
  GetFactory().Flush();

  return ShutdownFlag.load(std::memory_order_relaxed) || Registered.load(std::memory_order_relaxed) == false;
}

void ConsoleBackend::Flush()
{
  GetFactory().Flush();
}

const char* ConsoleBackend::GetEscapeSequence(Level level)
{
  if (level < Level::LEVEL_WARN)
    return nullptr;

  if (level == Level::LEVEL_WARN)
    return ANSI_YELLOW;

  return ANSI_LIGHT_RED;
}

FILE* ConsoleBackend::GetOutputStream(Context& context)
{
  OutputFlags flags = Owner->GetFlags();

  switch (flags.Console)
  {
    case ConsoleStream::STREAM_ALL2COUT:
      return stdout;

    case ConsoleStream::STREAM_ALL2CERR:
      return stderr;

    case ConsoleStream::STREAM_WARNCERR:
      return context.ErrorLevel >= Level::LEVEL_WARN ? stderr : stdout;

    case ConsoleStream::STREAM_ERRCERR:
      return context.ErrorLevel >= Level::LEVEL_ERROR? stderr : stdout;

    case ConsoleStream::STREAM_CERRCERR:
      return context.ErrorLevel >= Level::LEVEL_CRITICAL? stderr : stdout;
  }
  return stdout;
}

static void PrintWithAnsiSegments(
  FILE* stream
  , const char* text
  , const char* defaultEscape
)
{
  if (!text || !*text)
    return;

  bool isStdErr = (stream == stderr);

  Colorizer colorizer(isStdErr);

  // If VT mode is enabled we do not parse escapes ourselves.
  if (Colorizer::VTMode())
  {
    if (defaultEscape && *defaultEscape)
      colorizer.Escape(defaultEscape);

    fputs(text, stream);
    return;
  }

  const char* p = text;
  const char* escPos = nullptr;
  bool defaultApplied = false;

  while ((escPos = std::strchr(p, '\x1b')) != nullptr)
  {
    if (escPos > p)
    {
      size_t len = static_cast<size_t>(escPos - p);

      if (defaultEscape && !defaultApplied)
      {
        colorizer.Escape(defaultEscape);
        defaultApplied = true;
      }

      if (len > 0)
        fwrite(p, 1, len, stream);
    }

    const char* seq = escPos;
    int attr = 0;
    int fg = -1;
    int bg = -1;

    if (Colorizer::ParseSequence(seq, attr, fg, bg))
    {
      colorizer.Escape(escPos);
      defaultApplied = false;
      p = seq;
      continue;
    }
    else
    {
      fputc('\x1b', stream);
      p = escPos + 1;
      continue;
    }
  }

  if (*p)
  {
    if (defaultEscape && !defaultApplied)
    {
      colorizer.Escape(defaultEscape);
      defaultApplied = true;
    }

    fputs(p, stream);
  }

  // Colorizer dtor will restore original console attributes.
}

void ConsoleBackend::RemoveAnsi(const char* text, size_t len, std::string& out)
{
  out.clear();

  if (!text || len == 0)
    return;

  const char* p = text;
  const char* end = text + len;
  const char* escPos = nullptr;

  while (p < end && (escPos = static_cast<const char*>(memchr(p, '\x1b', static_cast<size_t>(end - p)))) != nullptr)
  {
    if (escPos > p)
      out.append(p, static_cast<size_t>(escPos - p));

    const char* seq = escPos;
    int attr = 0;
    int fg = -1;
    int bg = -1;

    if (Colorizer::ParseSequence(seq, attr, fg, bg))
    {
      p = seq;
      continue;
    }

    p = escPos + 1;
  }

  if (p < end)
    out.append(p, static_cast<size_t>(end - p));
}

static void PrintWithoutAnsi(
  FILE* stream
  , const char* text
  , size_t len
)
{
  std::string out;
  ConsoleBackend::RemoveAnsi(text, len, out);

  if (!out.empty())
    fwrite(out.data(), 1, out.size(), stream);
}

void ConsoleBackend::WriteText(FILE* stream, const char* text, size_t len, const char* escape)
{
  if (!text || len == 0)
    return;

  static std::mutex ConsoleOutputLock;
  std::lock_guard guard(ConsoleOutputLock);

  const bool hasAnsi = (memchr(text, '\x1b', len) != nullptr);
  const bool isTty = Colorizer::IsTTY(stream);

  if (isTty && (escape || hasAnsi))
  {
    std::string local;
    local.assign(text, len);

    if (hasAnsi)
    {
      PrintWithAnsiSegments(stream, local.c_str(), escape);
    }
    else if (escape)
    {
      Colorizer colorizer(stream == stderr);
      colorizer.Escape(escape);

      fwrite(text, 1, len, stream);
    }
  }
  else if (hasAnsi)
  {
    PrintWithoutAnsi(stream, text, len);
  }
  else
  {
    fwrite(text, 1, len, stream);
  }
}

ConsoleManagerFactory& ConsoleBackend::GetFactory() const
{
  auto logger = Owner->GetOwner();
  return logger->GetConsoleManagerFactory();
}

void ConsoleBackend::RegisterIfNeeded()
{
  if (Registered.load(std::memory_order_relaxed))
    return;

  ConsoleBackendPtr self = std::static_pointer_cast<ConsoleBackend>(shared_from_this());
  GetFactory().Add(self, QueueRecordLimit, QueueByteLimit, OverflowPolicy);
  Registered.store(true, std::memory_order_relaxed);
}

void ConsoleBackend::Display(Context& context)
{
  OutputFlags flags = Owner->GetFlags();
  if (context.Ovr)
  {
    flags.Value |= context.Ovr->Add.Value;
    flags.Value &= ~context.Ovr->Remove.Value;
  }

  int nc;
  const char* buffer = context.Apply(Owner, flags, nc);
  const char* escape = nullptr;

  if (flags.Highlight)
    escape = GetEscapeSequence(context.ErrorLevel);

  FILE* stream = GetOutputStream(context);
  ConsoleTarget target = stream == stderr ? ConsoleTarget::STDERR : ConsoleTarget::STDOUT;
  const bool isTty = Colorizer::IsTTY(stream);

  if (!isTty && GetFactory().AppendRedirected(Owner, target, buffer, static_cast<size_t>(nc)))
  {
    RegisterIfNeeded();
    return;
  }

  if (!Async || ShutdownFlag.load(std::memory_order_relaxed))
  {
    WriteText(stream, buffer, static_cast<size_t>(nc), escape);
    return;
  }

  RegisterIfNeeded();

  bool queued = GetFactory().Push(
    target
    , context.ErrorLevel
    , flags.Highlight
    , buffer
    , static_cast<size_t>(nc)
  );

  if (!queued)
    WriteText(stream, buffer, static_cast<size_t>(nc), escape);
}

void ConsoleBackend::OnShutdown()
{
  ShutdownCalled.store(true, std::memory_order_relaxed);
}
