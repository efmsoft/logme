#include <algorithm>
#include <chrono>
#include <cstring>

#include <Logme/WindowsEventLog/WindowsEventLogManager.h>

using namespace Logme;

namespace
{
  enum
  {
    WINDOWS_EVENT_LOG_QUEUE_RECORD_LIMIT = 8192,
    WINDOWS_EVENT_LOG_QUEUE_BYTE_LIMIT = 4 * 1024 * 1024,
  };
}

WindowsEventLogManager::WindowsEventLogManager()
  : StopRequested(false)
  , Reschedule(false)
  , Processing(false)
  , QueuedRecords(0)
  , QueuedBytes(0)
{
  Buffer.reserve(256 * 1024);
}

WindowsEventLogManager::~WindowsEventLogManager()
{
  SetStopping();

  if (ManagerThread.joinable())
    ManagerThread.join();

  Flush();

  for (const auto& backend : Backends)
    backend->OnShutdown();
}

void WindowsEventLogManager::AddBackend(const WindowsEventLogBackendPtr& backend)
{
  std::lock_guard lock(Lock);
  Backends.insert(backend.get());

  if (!ManagerThread.joinable())
    ManagerThread = std::thread(&WindowsEventLogManager::ManagementThread, this);
}

bool WindowsEventLogManager::RemoveBackend(WindowsEventLogBackend* backend)
{
  bool empty = false;

  {
    std::unique_lock lock(Lock);

    while (Processing && WorkerAvailable())
    {
      NotFull.wait_for(
        lock
        , std::chrono::milliseconds(100)
      );
    }

    DrainPending(lock);

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

bool WindowsEventLogManager::HasSpace(size_t recordSize) const
{
  bool recordsOk = QueuedRecords < WINDOWS_EVENT_LOG_QUEUE_RECORD_LIMIT;
  bool bytesOk = QueuedBytes == 0 || QueuedBytes + recordSize <= WINDOWS_EVENT_LOG_QUEUE_BYTE_LIMIT;
  return recordsOk && bytesOk;
}

bool WindowsEventLogManager::Push(WindowsEventLogBackend* backend, Level level, const char* text, size_t len)
{
  if (!backend || !text || len == 0)
    return true;

  size_t recordSize = sizeof(WindowsEventLogRecordHeader) + len + 1;
  if (recordSize > UINT32_MAX)
    return false;

  WindowsEventLogRecordHeader header;
  header.Size = static_cast<uint32_t>(recordSize);
  header.TextSize = static_cast<uint32_t>(len);
  header.EventId = backend->GetEventId();
  header.Category = backend->GetCategory();
  header.Level = static_cast<uint16_t>(level);
  header.Backend = backend;

  std::unique_lock lock(Lock);

  auto hasSpace = [this, recordSize]()
  {
    return HasSpace(recordSize);
  };

  while (!hasSpace() && !Stopping() && WorkerAvailable())
  {
    NotFull.wait_for(
      lock
      , std::chrono::milliseconds(100)
    );
  }

  if (!hasSpace())
    DrainPending(lock);

  if (Stopping() || !hasSpace())
    return false;

  size_t oldSize = Buffer.size();
  Buffer.resize(oldSize + recordSize);
  memcpy(Buffer.data() + oldSize, &header, sizeof(header));
  memcpy(Buffer.data() + oldSize + sizeof(header), text, len);
  Buffer[oldSize + sizeof(header) + len] = 0;

  QueuedRecords++;
  QueuedBytes += recordSize;

  Reschedule = true;
  lock.unlock();
  CV.notify_one();
  return true;
}

bool WindowsEventLogManager::PushAndFlush(WindowsEventLogBackend* backend, Level level, const char* text, size_t len)
{
  if (!Push(backend, level, text, len))
    return false;

  Flush();
  return true;
}

void WindowsEventLogManager::Flush()
{
  std::unique_lock lock(Lock);

  while ((!Buffer.empty() || Processing) && WorkerAvailable())
  {
    CV.notify_one();
    NotFull.wait_for(
      lock
      , std::chrono::milliseconds(100)
    );
  }

  if (!Buffer.empty())
    DrainPending(lock);
}

bool WindowsEventLogManager::Empty() const
{
  std::lock_guard lock(Lock);
  return Buffer.empty() && !Processing;
}

bool WindowsEventLogManager::Stopping() const
{
  return StopRequested.load(std::memory_order_relaxed);
}

void WindowsEventLogManager::SetStopping()
{
  StopRequested.store(true, std::memory_order_relaxed);
  CV.notify_all();
  NotFull.notify_all();
}

size_t WindowsEventLogManager::GetMemoryUsage() const
{
  std::lock_guard lock(Lock);
  return Buffer.capacity();
}

bool WindowsEventLogManager::WorkerAvailable() const
{
  return ManagerThread.joinable() && !StopRequested.load(std::memory_order_relaxed);
}

void WindowsEventLogManager::DrainPending(std::unique_lock<std::mutex>& lock)
{
  if (Buffer.empty())
    return;

  std::vector<unsigned char> local;
  local.swap(Buffer);
  QueuedRecords = 0;
  QueuedBytes = 0;
  Processing = true;

  lock.unlock();
  ProcessBuffer(local);
  lock.lock();

  Processing = false;
  NotFull.notify_all();
}

void WindowsEventLogManager::ManagementThread()
{
  for (;;)
  {
    std::vector<unsigned char> local;

    {
      std::unique_lock lock(Lock);
      CV.wait(
        lock
        , [this]()
        {
          return StopRequested.load(std::memory_order_relaxed) || Reschedule;
        }
      );

      Reschedule = false;

      if (StopRequested.load(std::memory_order_relaxed) && Buffer.empty())
        break;

      if (Buffer.empty())
        continue;

      local.swap(Buffer);
      QueuedRecords = 0;
      QueuedBytes = 0;
      Processing = true;
      NotFull.notify_all();
    }

    ProcessBuffer(local);

    {
      std::lock_guard lock(Lock);
      Processing = false;
      NotFull.notify_all();
    }
  }
}

void WindowsEventLogManager::ProcessBuffer(std::vector<unsigned char>& buffer)
{
  size_t pos = 0;

  while (pos + sizeof(WindowsEventLogRecordHeader) <= buffer.size())
  {
    auto header = reinterpret_cast<const WindowsEventLogRecordHeader*>(buffer.data() + pos);
    if (header->Size == 0 || pos + header->Size > buffer.size())
      break;

    const char* text = reinterpret_cast<const char*>(buffer.data() + pos + sizeof(WindowsEventLogRecordHeader));
    header->Backend->WritePreparedRecord(
      static_cast<Level>(header->Level)
      , header->EventId
      , header->Category
      , text
      , header->TextSize
    );
    pos += header->Size;
  }
}
