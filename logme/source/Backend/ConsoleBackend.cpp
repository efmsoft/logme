#include <chrono>
#include <cstdio>
#include <cstring>

#include <Logme/AnsiColorEscape.h>
#include <Logme/Backend/ConsoleBackend.h>
#include <Logme/Backend/ConsoleManagerFactory.h>
#include <Logme/Channel.h>
#include <Logme/Colorizer.h>
#include <Logme/File/exe_path.h>
#include <Logme/Logger.h>

using namespace Logme;

ConsoleRecordQueue::ConsoleRecordQueue()
  : QueuedBytes(0)
  , MaxRecords(ConsoleBackend::QUEUE_RECORD_LIMIT)
  , MaxBytes(ConsoleBackend::QUEUE_BYTE_LIMIT)
  , OverflowPolicy(ConsoleOverflowPolicy::BLOCK)
  , DroppedRecords(0)
  , DroppedBytes(0)
  , BlockedCalls(0)
  , SyncFallbackCalls(0)
  , MaxQueuedRecords(0)
  , MaxQueuedBytes(0)
{
  Records.reserve(256);
}

void ConsoleRecordQueue::SetLimits(size_t maxRecords, size_t maxBytes)
{
  std::lock_guard guard(Lock);
  MaxRecords = maxRecords;
  MaxBytes = maxBytes;
  NotFull.notify_all();
}

void ConsoleRecordQueue::SetOverflowPolicy(ConsoleOverflowPolicy policy)
{
  std::lock_guard guard(Lock);
  OverflowPolicy = policy;
  NotFull.notify_all();
}

bool ConsoleRecordQueue::Push(ConsoleRecord&& record)
{
  const size_t bytes = record.Text.size();
  std::unique_lock guard(Lock);

  auto hasSpace = [this, bytes]()
  {
    bool recordsOk = MaxRecords == 0 || Records.size() < MaxRecords;
    bool bytesOk = MaxBytes == 0 || Records.empty() || QueuedBytes + bytes <= MaxBytes;
    return recordsOk && bytesOk;
  };

  if (!hasSpace())
  {
    switch (OverflowPolicy)
    {
      case ConsoleOverflowPolicy::BLOCK:
        ++BlockedCalls;
        NotFull.wait(guard, hasSpace);
        break;

      case ConsoleOverflowPolicy::DROP_NEW:
        ++DroppedRecords;
        DroppedBytes += bytes;
        return true;

      case ConsoleOverflowPolicy::DROP_OLDEST:
        while (!Records.empty() && !hasSpace())
        {
          size_t droppedBytes = Records.front().Text.size();
          QueuedBytes -= droppedBytes;
          ++DroppedRecords;
          DroppedBytes += droppedBytes;
          Records.erase(Records.begin());
        }
        if (!hasSpace())
        {
          ++DroppedRecords;
          DroppedBytes += bytes;
          return true;
        }
        break;

      case ConsoleOverflowPolicy::SYNC_FALLBACK:
        ++SyncFallbackCalls;
        return false;
    }
  }

  QueuedBytes += bytes;
  Records.emplace_back(std::move(record));

  if (Records.size() > MaxQueuedRecords)
    MaxQueuedRecords = Records.size();

  if (QueuedBytes > MaxQueuedBytes)
    MaxQueuedBytes = QueuedBytes;
  return true;
}

bool ConsoleRecordQueue::TakeAll(std::vector<ConsoleRecord>& records)
{
  std::lock_guard guard(Lock);

  if (Records.empty())
    return false;

  records.swap(Records);
  QueuedBytes = 0;
  NotFull.notify_all();
  return true;
}

bool ConsoleRecordQueue::Empty() const
{
  auto self = const_cast<ConsoleRecordQueue*>(this);
  std::lock_guard guard(self->Lock);
  return self->Records.empty();
}

ConsoleQueueCounters ConsoleRecordQueue::GetCounters() const
{
  auto self = const_cast<ConsoleRecordQueue*>(this);
  std::lock_guard guard(self->Lock);

  ConsoleQueueCounters counters;
  counters.QueuedRecords = self->Records.size();
  counters.QueuedBytes = self->QueuedBytes;
  counters.MaxQueuedRecords = self->MaxQueuedRecords;
  counters.MaxQueuedBytes = self->MaxQueuedBytes;
  counters.DroppedRecords = self->DroppedRecords;
  counters.DroppedBytes = self->DroppedBytes;
  counters.BlockedCalls = self->BlockedCalls;
  counters.SyncFallbackCalls = self->SyncFallbackCalls;
  return counters;
}

void ConsoleRecordQueue::NotifySpace()
{
  NotFull.notify_all();
}

ConsoleBackend::ConsoleBackend(ChannelPtr owner)
  : Backend(owner, TYPE_ID)
  , Async(false)
  , Registered(false)
  , ShutdownFlag(false)
  , ShutdownCalled(owner == nullptr)
  , RedirectStdoutChecked(false)
  , RedirectStderrChecked(false)
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

  if (Registered.load(std::memory_order_relaxed) && ShutdownCalled.load(std::memory_order_relaxed) == false)
  {
    std::unique_lock locker(DoneLock);
    Done.wait_for(locker, std::chrono::milliseconds(50));
  }
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
  Queue.SetLimits(maxRecords, maxBytes);
}

void ConsoleBackend::SetOverflowPolicy(ConsoleOverflowPolicy policy)
{
  Queue.SetOverflowPolicy(policy);
}

ConsoleQueueCounters ConsoleBackend::GetQueueCounters() const
{
  return Queue.GetCounters();
}

void ConsoleBackend::Freeze()
{
  ShutdownFlag.store(true, std::memory_order_relaxed);
  Backend::Freeze();

  if (Registered.load(std::memory_order_relaxed))
    GetFactory().Notify(this);

  if (RedirectStdout)
    RedirectStdout->Freeze();

  if (RedirectStderr)
    RedirectStderr->Freeze();
}

bool ConsoleBackend::IsIdle() const
{
  bool redirectsIdle = true;

  if (RedirectStdout && !RedirectStdout->IsIdle())
    redirectsIdle = false;

  if (RedirectStderr && !RedirectStderr->IsIdle())
    redirectsIdle = false;

  return Queue.Empty() && redirectsIdle && (ShutdownFlag.load(std::memory_order_relaxed) || Registered.load(std::memory_order_relaxed) == false);
}

void ConsoleBackend::Flush()
{
  if (RedirectStdout)
    RedirectStdout->Flush();

  if (RedirectStderr)
    RedirectStderr->Flush();

  if (!Registered.load(std::memory_order_relaxed))
    return;

  GetFactory().Notify(this);

  std::unique_lock locker(DoneLock);
  Done.wait(locker, [this]() { return Queue.Empty(); });
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

  const bool hasAnsi = (memchr(text, '\x1b', len) != nullptr);
  const bool isTty = Colorizer::IsTTY(stream);

  if (isTty && (escape || hasAnsi))
  {
    static std::mutex ColorizerLock;
    std::lock_guard guard(ColorizerLock);

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

FileBackendPtr ConsoleBackend::GetRedirectBackend(ConsoleTarget target)
{
  FileBackendPtr& backend = target == ConsoleTarget::STDOUT ? RedirectStdout : RedirectStderr;
  bool& checked = target == ConsoleTarget::STDOUT ? RedirectStdoutChecked : RedirectStderrChecked;

  if (checked)
    return backend;

  checked = true;

  FILE* stream = target == ConsoleTarget::STDOUT ? stdout : stderr;
  std::string name = GetFilePathFromFile(stream);
  if (name.empty())
    return nullptr;

  backend = std::make_shared<FileBackend>(Owner);
  backend->SetAppend(true);
  backend->SetMaxSize(0);
  if (!backend->CreateLog(name.c_str()))
    backend.reset();

  return backend;
}

bool ConsoleBackend::AppendRedirected(ConsoleTarget target, const char* text, size_t len, bool hasAnsi)
{
  FileBackendPtr backend = GetRedirectBackend(target);
  if (!backend)
    return false;

  if (hasAnsi)
  {
    std::string plain;
    RemoveAnsi(text, len, plain);
    backend->AppendString(plain.data(), plain.size());
  }
  else
  {
    backend->AppendString(text, len);
  }

  return true;
}

void ConsoleBackend::RegisterIfNeeded()
{
  if (Registered.load(std::memory_order_relaxed))
    return;

  ConsoleBackendPtr self = std::static_pointer_cast<ConsoleBackend>(shared_from_this());
  GetFactory().Add(self);
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
  const bool hasAnsi = (memchr(buffer, '\x1b', static_cast<size_t>(nc)) != nullptr);

  if (!Async || ShutdownFlag.load(std::memory_order_relaxed))
  {
    WriteText(stream, buffer, static_cast<size_t>(nc), escape);
    return;
  }

  ConsoleTarget target = stream == stderr ? ConsoleTarget::STDERR : ConsoleTarget::STDOUT;
  const bool isTty = Colorizer::IsTTY(stream);

  if (!isTty && AppendRedirected(target, buffer, static_cast<size_t>(nc), hasAnsi))
    return;

  RegisterIfNeeded();

  ConsoleRecord record;
  record.Target = target;
  record.ErrorLevel = context.ErrorLevel;
  record.Highlight = flags.Highlight;
  record.HasAnsi = hasAnsi;
  record.Text.assign(buffer, static_cast<size_t>(nc));

  bool queued = Queue.Push(std::move(record));
  if (queued)
  {
    GetFactory().Notify(this);
    return;
  }

  if (!queued)
    WriteText(stream, buffer, static_cast<size_t>(nc), escape);
}

bool ConsoleBackend::WorkerFunc()
{
  std::vector<ConsoleRecord> records;
  bool hasRecords = Queue.TakeAll(records);

  if (!hasRecords)
  {
    std::lock_guard guard(DoneLock);
    Done.notify_all();
    return !ShutdownFlag.load(std::memory_order_relaxed);
  }

  for (auto& record : records)
  {
    FILE* stream = record.Target == ConsoleTarget::STDERR ? stderr : stdout;
    const char* escape = nullptr;

    if (record.Highlight)
      escape = GetEscapeSequence(record.ErrorLevel);

    WriteText(stream, record.Text.data(), record.Text.size(), escape);
  }

  {
    std::lock_guard guard(DoneLock);
    Done.notify_all();
  }

  return !ShutdownFlag.load(std::memory_order_relaxed) || !Queue.Empty();
}

void ConsoleBackend::OnShutdown()
{
  ShutdownCalled.store(true, std::memory_order_relaxed);

  std::lock_guard guard(DoneLock);
  Done.notify_all();
}
