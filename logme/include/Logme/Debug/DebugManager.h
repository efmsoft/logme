#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include <Logme/Backend/DebugBackend.h>

namespace Logme
{
  struct DebugRecordHeader
  {
    uint32_t Size;
    uint32_t TextSize;
  };

  class DebugManager
  {
    friend struct DebugBackend;

    std::atomic<bool> StopRequested;
    bool Reschedule;
    std::thread ManagerThread;

    mutable std::mutex Lock;
    std::condition_variable CV;
    std::condition_variable NotFull;
    std::set<DebugBackend*> Backends;
    std::vector<unsigned char> Buffer;
    bool Processing;
    size_t QueuedRecords;
    size_t QueuedBytes;

  public:
    DebugManager();
    ~DebugManager();

    void AddBackend(const DebugBackendPtr& backend);
    bool RemoveBackend(DebugBackend* backend);
    bool Push(const char* text, size_t len);
    bool PushAndFlush(const char* text, size_t len);
    void Flush();
    bool Empty() const;
    bool Stopping() const;
    void SetStopping();
    size_t GetMemoryUsage() const;

  private:
    bool HasSpace(size_t recordSize) const;
    bool WorkerAvailable() const;
    void DrainPending(std::unique_lock<std::mutex>& lock);
    void ManagementThread();
    void ProcessBuffer(std::vector<unsigned char>& buffer);
  };
}
