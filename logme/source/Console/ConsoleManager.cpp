#include <algorithm>
#include <cstring>

#include <Logme/AnsiColorEscape.h>
#include <Logme/Console/ConsoleManager.h>
#include <Logme/File/exe_path.h>
#include <Logme/Utils.h>

using namespace Logme;

ConsoleManager::ConsoleManager()
  : StopRequested(false)
  , Reschedule(false)
  , Processing(false)
  , QueuedRecords(0)
  , QueuedBytes(0)
  , MaxRecords(ConsoleBackend::QUEUE_RECORD_LIMIT)
  , MaxBytes(ConsoleBackend::QUEUE_BYTE_LIMIT)
  , OverflowPolicy(ConsoleOverflowPolicy::BLOCK)
#if CONSOLE_ENABLE_COUNTERS
  , Counters{}
#endif
  , RedirectStdoutChecked(false)
  , RedirectStderrChecked(false)
#ifdef _WIN32
  , ThreadID(0)
#endif
{
  Buffer.reserve(256 * 1024);
}

ConsoleManager::~ConsoleManager()
{
  SetStopping();

  if (ManagerThread.joinable())
    ManagerThread.join();

  Flush();

  for (const auto& backend : Backends)
    backend->OnShutdown();

  ShutdownRedirectBackends();

#if CONSOLE_ENABLE_COUNTERS
  {
    const ConsoleQueueCounters counters = GetCounters();

    printf(
      "[ConsoleManager] "
      "QueuedRecords=%llu "
      "QueuedBytes=%llu "
      "MaxQueuedRecords=%llu "
      "MaxQueuedBytes=%llu "
      "DroppedRecords=%llu "
      "DroppedBytes=%llu "
      "BlockedCalls=%llu "
      "SyncFallbackCalls=%llu "
      "RedirectedRecords=%llu "
      "RedirectedBytes=%llu\n"
      , (unsigned long long)counters.QueuedRecords
      , (unsigned long long)counters.QueuedBytes
      , (unsigned long long)counters.MaxQueuedRecords
      , (unsigned long long)counters.MaxQueuedBytes
      , (unsigned long long)counters.DroppedRecords
      , (unsigned long long)counters.DroppedBytes
      , (unsigned long long)counters.BlockedCalls
      , (unsigned long long)counters.SyncFallbackCalls
      , (unsigned long long)counters.RedirectedRecords
      , (unsigned long long)counters.RedirectedBytes
    );
  }
#endif
}

void ConsoleManager::ShutdownRedirectBackends()
{
  FileBackendPtr stdoutBackend;
  FileBackendPtr stderrBackend;

  {
    std::lock_guard lock(Lock);
    stdoutBackend = RedirectStdout;
    stderrBackend = RedirectStderr;

    RedirectStdout.reset();
    RedirectStderr.reset();
    RedirectStdoutChecked = false;
    RedirectStderrChecked = false;
    RedirectStdoutName.clear();
    RedirectStderrName.clear();
  }

  if (stdoutBackend)
    stdoutBackend->Freeze();

  if (stderrBackend && stderrBackend != stdoutBackend)
    stderrBackend->Freeze();
}

void ConsoleManager::AddBackend(
  const ConsoleBackendPtr& backend
  , size_t maxRecords
  , size_t maxBytes
  , ConsoleOverflowPolicy policy
)
{
  std::lock_guard lock(Lock);
  Backends.insert(backend.get());
  MaxRecords = maxRecords;
  MaxBytes = maxBytes;
  OverflowPolicy = policy;

  if (!ManagerThread.joinable())
    ManagerThread = std::thread(&ConsoleManager::ManagementThread, this);
}

bool ConsoleManager::RemoveBackend(ConsoleBackend* backend)
{
  bool empty = false;

  {
    std::lock_guard lock(Lock);

    auto pos = Backends.find(backend);
    if (pos != Backends.end())
    {
      (*pos)->OnShutdown();
      Backends.erase(pos);
    }

    empty = Backends.empty();
    if (empty)
    {
      StopRequested.store(true, std::memory_order_relaxed);
      Reschedule = true;
    }
  }

  if (empty)
  {
    CV.notify_all();
    NotFull.notify_all();
  }

  return empty;
}

bool ConsoleManager::HasSpace(size_t recordSize) const
{
  bool recordsOk = MaxRecords == 0 || QueuedRecords < MaxRecords;
  bool bytesOk = MaxBytes == 0 || QueuedBytes == 0 || QueuedBytes + recordSize <= MaxBytes;
  return recordsOk && bytesOk;
}

bool ConsoleManager::DropOldest(size_t recordSize)
{
  while (!Buffer.empty() && !HasSpace(recordSize))
  {
    auto header = reinterpret_cast<const ConsoleRecordHeader*>(Buffer.data());
    if (header->Size == 0 || header->Size > Buffer.size())
      return false;

    CONSOLE_CNT(Counters.DroppedRecords++);
    CONSOLE_CNT(Counters.DroppedBytes += header->Size);

    Buffer.erase(Buffer.begin(), Buffer.begin() + header->Size);
    QueuedRecords--;
    QueuedBytes -= header->Size;
  }

  return HasSpace(recordSize);
}

bool ConsoleManager::Push(
  ConsoleTarget target
  , Level level
  , bool highlight
  , const char* text
  , size_t len
)
{
  if (!text || len == 0)
    return true;

  size_t recordSize = sizeof(ConsoleRecordHeader) + len;
  if (recordSize > UINT32_MAX)
    return false;

  ConsoleRecordHeader header;
  header.Size = static_cast<uint32_t>(recordSize);
  header.TextSize = static_cast<uint32_t>(len);
  header.Target = target == ConsoleTarget::STDERR ? 1 : 0;
  header.Flags = highlight ? CONSOLE_RECORD_HIGHLIGHT : 0;
  header.Reserved = 0;
  header.ErrorLevel = level;

  std::unique_lock lock(Lock);

  auto hasSpace = [this, recordSize]()
  {
    return HasSpace(recordSize);
  };

  if (!hasSpace())
  {
    switch (OverflowPolicy)
    {
      case ConsoleOverflowPolicy::BLOCK:
        CONSOLE_CNT(Counters.BlockedCalls++);
        NotFull.wait(lock, hasSpace);
        break;

      case ConsoleOverflowPolicy::DROP_NEW:
        CONSOLE_CNT(Counters.DroppedRecords++);
        CONSOLE_CNT(Counters.DroppedBytes += recordSize);
        return true;

      case ConsoleOverflowPolicy::DROP_OLDEST:
        if (!DropOldest(recordSize))
        {
          CONSOLE_CNT(Counters.DroppedRecords++);
          CONSOLE_CNT(Counters.DroppedBytes += recordSize);
          return true;
        }
        break;

      case ConsoleOverflowPolicy::SYNC_FALLBACK:
        CONSOLE_CNT(Counters.SyncFallbackCalls++);
        return false;
    }
  }

  size_t oldSize = Buffer.size();
  Buffer.resize(oldSize + recordSize);
  memcpy(Buffer.data() + oldSize, &header, sizeof(header));
  memcpy(Buffer.data() + oldSize + sizeof(header), text, len);

  QueuedRecords++;
  QueuedBytes += recordSize;
  CONSOLE_CNT(Counters.QueuedRecords = QueuedRecords);
  CONSOLE_CNT(Counters.QueuedBytes = QueuedBytes);
  CONSOLE_CNT(Counters.MaxQueuedRecords = std::max<uint64_t>(Counters.MaxQueuedRecords, QueuedRecords));
  CONSOLE_CNT(Counters.MaxQueuedBytes = std::max<uint64_t>(Counters.MaxQueuedBytes, QueuedBytes));

  Reschedule = true;
  lock.unlock();
  CV.notify_one();
  return true;
}

FileBackendPtr ConsoleManager::GetRedirectBackend(
  const ChannelPtr& owner
  , ConsoleTarget target
)
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

  if (target == ConsoleTarget::STDOUT)
  {
    if (RedirectStderrChecked && RedirectStderr && name == RedirectStderrName)
    {
      backend = RedirectStderr;
      RedirectStdoutName = name;
      return backend;
    }

    RedirectStdoutName = name;
  }
  else
  {
    if (RedirectStdoutChecked && RedirectStdout && name == RedirectStdoutName)
    {
      backend = RedirectStdout;
      RedirectStderrName = name;
      return backend;
    }

    RedirectStderrName = name;
  }

  backend = std::make_shared<FileBackend>(owner);
  backend->SetAppend(true);
  backend->SetMaxSize(0);
  if (!backend->CreateLog(name.c_str()))
    backend.reset();

  return backend;
}

bool ConsoleManager::AppendRedirected(
  const ChannelPtr& owner
  , ConsoleTarget target
  , const char* text
  , size_t len
)
{
  if (!text || len == 0)
    return true;

  FileBackendPtr backend;

  {
    std::lock_guard lock(Lock);
    backend = GetRedirectBackend(owner, target);
  }

  if (!backend)
    return false;

  const bool hasAnsi = (memchr(text, '\x1b', len) != nullptr);

  if (hasAnsi)
  {
    std::string plain;
    ConsoleBackend::RemoveAnsi(text, len, plain);
    backend->AppendString(plain.data(), plain.size());
  }
  else
  {
    backend->AppendString(text, len);
  }

  {
    std::lock_guard lock(Lock);
    CONSOLE_CNT(Counters.RedirectedRecords++);
    CONSOLE_CNT(Counters.RedirectedBytes += len);
  }

  return true;
}

void ConsoleManager::Flush()
{
  FileBackendPtr stdoutBackend;
  FileBackendPtr stderrBackend;

  {
    std::unique_lock lock(Lock);
    CV.notify_one();
    NotFull.wait(lock, [this]() { return Buffer.empty() && !Processing; });

    stdoutBackend = RedirectStdout;
    stderrBackend = RedirectStderr;
  }

  if (stdoutBackend)
    stdoutBackend->Flush();

  if (stderrBackend && stderrBackend != stdoutBackend)
    stderrBackend->Flush();
}

bool ConsoleManager::Empty() const
{
  std::lock_guard lock(Lock);
  return Buffer.empty() && !Processing;
}

void ConsoleManager::SetLimits(size_t maxRecords, size_t maxBytes)
{
  std::lock_guard lock(Lock);
  MaxRecords = maxRecords;
  MaxBytes = maxBytes;
  NotFull.notify_all();
}

void ConsoleManager::SetOverflowPolicy(ConsoleOverflowPolicy policy)
{
  std::lock_guard lock(Lock);
  OverflowPolicy = policy;
  NotFull.notify_all();
}

ConsoleQueueCounters ConsoleManager::GetCounters() const
{
#if CONSOLE_ENABLE_COUNTERS
  std::lock_guard lock(Lock);
  return Counters;
#else
  return ConsoleQueueCounters{};
#endif
}

bool ConsoleManager::Stopping() const
{
  return StopRequested.load(std::memory_order_relaxed);
}

#ifdef _WIN32
#include <windows.h>

static bool ThreadExists(uint64_t threadId)
{
  HANDLE hThread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(threadId));
  if (!hThread)
    return false;

  DWORD exitCode = 0;
  BOOL ok = GetExitCodeThread(hThread, &exitCode);
  CloseHandle(hThread);

  if (!ok)
    return false;

  return exitCode == STILL_ACTIVE;
}
#endif

void ConsoleManager::SetStopping()
{
  if (true)
  {
    std::lock_guard<std::mutex> lock(Lock);

    StopRequested.store(true, std::memory_order_relaxed);
    Reschedule = true;
  }

  CV.notify_all();
  NotFull.notify_all();

#ifdef _WIN32
  if (ThreadExists(ThreadID) == false)
  {
    for (const auto& backend : Backends)
      backend->OnShutdown();

    Backends.clear();
  }
#endif
}

void ConsoleManager::Notify(ConsoleBackend* backend)
{
  (void)backend;

  {
    std::lock_guard<std::mutex> lock(Lock);
    Reschedule = true;
  }

  CV.notify_all();
}

static void FlushPlainBatch(FILE*& batchStream, std::string& batch)
{
  if (!batch.empty() && batchStream)
    ConsoleBackend::WriteText(batchStream, batch.data(), batch.size(), nullptr);

  batch.clear();
  batchStream = nullptr;
}

void ConsoleManager::ProcessBuffer(std::vector<unsigned char>& buffer)
{
  const unsigned char* pos = buffer.data();
  const unsigned char* end = pos + buffer.size();
  FILE* batchStream = nullptr;
  std::string batch;

  while (pos < end)
  {
    if (static_cast<size_t>(end - pos) < sizeof(ConsoleRecordHeader))
      break;

    auto header = reinterpret_cast<const ConsoleRecordHeader*>(pos);
    if (header->Size < sizeof(ConsoleRecordHeader) || pos + header->Size > end)
      break;

    const char* text = reinterpret_cast<const char*>(pos + sizeof(ConsoleRecordHeader));
    FILE* stream = header->Target ? stderr : stdout;
    const bool highlight = (header->Flags & CONSOLE_RECORD_HIGHLIGHT) != 0;
    const bool hasAnsi = memchr(text, '\x1b', header->TextSize) != nullptr;
    const char* escape = nullptr;

    if (highlight)
    {
      Level level = static_cast<Level>(header->ErrorLevel);
      if (level >= Level::LEVEL_WARN)
        escape = level == Level::LEVEL_WARN ? ANSI_YELLOW : ANSI_LIGHT_RED;
    }

    if (!highlight && !hasAnsi)
    {
      if (batchStream != stream)
        FlushPlainBatch(batchStream, batch);

      batchStream = stream;
      batch.append(text, header->TextSize);
    }
    else
    {
      FlushPlainBatch(batchStream, batch);
      ConsoleBackend::WriteText(stream, text, header->TextSize, escape);
    }

    pos += header->Size;
  }

  FlushPlainBatch(batchStream, batch);
}

void ConsoleManager::ManagementThread()
{
  RenameThread(uint64_t(-1), "ConsoleManager::ManagementThread");

#ifdef _WIN32
  ThreadID = GetCurrentThreadId();
#endif

  for (;;)
  {
    std::vector<unsigned char> buffer;
    bool stopping = false;

    {
      std::unique_lock<std::mutex> lock(Lock);
      CV.wait(lock, [this]() { return StopRequested.load(std::memory_order_relaxed) || Reschedule || !Buffer.empty(); });

      Reschedule = false;
      stopping = StopRequested.load(std::memory_order_relaxed);
      buffer.swap(Buffer);
      Processing = !buffer.empty();
      QueuedRecords = 0;
      QueuedBytes = 0;
      CONSOLE_CNT(Counters.QueuedRecords = 0);
      CONSOLE_CNT(Counters.QueuedBytes = 0);
      NotFull.notify_all();
    }

    if (!buffer.empty())
      ProcessBuffer(buffer);

    {
      std::lock_guard<std::mutex> lock(Lock);
      Processing = false;
      NotFull.notify_all();
    }

    if (stopping)
    {
      std::lock_guard<std::mutex> lock(Lock);
      if (Buffer.empty())
        break;

      Reschedule = true;
    }
  }

#ifdef _WIN32
  ThreadID = 0;
#endif
}
