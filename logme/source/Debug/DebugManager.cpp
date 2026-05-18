#include <algorithm>
#include <chrono>
#include <cstring>

#include <Logme/Debug/DebugManager.h>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace Logme;

namespace
{
  enum
  {
    DEBUG_QUEUE_RECORD_LIMIT = 8192,
    DEBUG_QUEUE_BYTE_LIMIT = 4 * 1024 * 1024,
  };

  static void WriteDebugText(const char* text)
  {
#ifdef _WIN32
    OutputDebugStringA(text);
#else
    (void)text;
#endif
  }
}

DebugManager::DebugManager()
  : StopRequested(false)
  , Reschedule(false)
  , Processing(false)
  , QueuedRecords(0)
  , QueuedBytes(0)
{
  Buffer.reserve(256 * 1024);
}

DebugManager::~DebugManager()
{
  SetStopping();

  if (ManagerThread.joinable())
    ManagerThread.join();

  Flush();

  for (const auto& backend : Backends)
    backend->OnShutdown();
}

void DebugManager::AddBackend(const DebugBackendPtr& backend)
{
  std::lock_guard lock(Lock);
  Backends.insert(backend.get());

  if (!ManagerThread.joinable())
    ManagerThread = std::thread(&DebugManager::ManagementThread, this);
}

bool DebugManager::RemoveBackend(DebugBackend* backend)
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

bool DebugManager::HasSpace(size_t recordSize) const
{
  bool recordsOk = QueuedRecords < DEBUG_QUEUE_RECORD_LIMIT;
  bool bytesOk = QueuedBytes == 0 || QueuedBytes + recordSize <= DEBUG_QUEUE_BYTE_LIMIT;
  return recordsOk && bytesOk;
}

bool DebugManager::Push(const char* text, size_t len)
{
  if (!text || len == 0)
    return true;

  size_t recordSize = sizeof(DebugRecordHeader) + len + 1;
  if (recordSize > UINT32_MAX)
    return false;

  DebugRecordHeader header;
  header.Size = static_cast<uint32_t>(recordSize);
  header.TextSize = static_cast<uint32_t>(len);

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

bool DebugManager::PushAndFlush(const char* text, size_t len)
{
  if (!Push(text, len))
    return false;

  Flush();
  return true;
}

void DebugManager::Flush()
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

bool DebugManager::Empty() const
{
  std::lock_guard lock(Lock);
  return Buffer.empty() && !Processing;
}

bool DebugManager::Stopping() const
{
  return StopRequested.load(std::memory_order_relaxed);
}

void DebugManager::SetStopping()
{
  StopRequested.store(true, std::memory_order_relaxed);
  CV.notify_all();
  NotFull.notify_all();
}

size_t DebugManager::GetMemoryUsage() const
{
  std::lock_guard lock(Lock);
  return Buffer.capacity();
}

bool DebugManager::WorkerAvailable() const
{
  return ManagerThread.joinable() && !StopRequested.load(std::memory_order_relaxed);
}

void DebugManager::DrainPending(std::unique_lock<std::mutex>& lock)
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

void DebugManager::ManagementThread()
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

void DebugManager::ProcessBuffer(std::vector<unsigned char>& buffer)
{
  size_t pos = 0;

  while (pos + sizeof(DebugRecordHeader) <= buffer.size())
  {
    auto header = reinterpret_cast<const DebugRecordHeader*>(buffer.data() + pos);
    if (header->Size == 0 || pos + header->Size > buffer.size())
      break;

    const char* text = reinterpret_cast<const char*>(buffer.data() + pos + sizeof(DebugRecordHeader));
    WriteDebugText(text);
    pos += header->Size;
  }
}
