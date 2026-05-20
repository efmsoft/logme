#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include <Logme/Backend/WindowsEventLogBackend.h>

namespace Logme
{
  struct WindowsEventLogRecordHeader
  {
    uint32_t Size;
    uint32_t TextSize;
    uint32_t EventId;
    uint16_t Category;
    uint16_t Level;
    WindowsEventLogBackend* Backend;
  };

  class WindowsEventLogManager
  {
    friend struct WindowsEventLogBackend;

    std::atomic<bool> StopRequested;
    bool Reschedule;
    std::thread ManagerThread;

    mutable std::mutex Lock;
    std::condition_variable CV;
    std::condition_variable NotFull;
    std::set<WindowsEventLogBackend*> Backends;
    std::vector<unsigned char> Buffer;
    bool Processing;
    size_t QueuedRecords;
    size_t QueuedBytes;

  public:
    WindowsEventLogManager();
    ~WindowsEventLogManager();

    void AddBackend(const WindowsEventLogBackendPtr& backend);
    bool RemoveBackend(WindowsEventLogBackend* backend);
    bool Push(WindowsEventLogBackend* backend, Level level, const char* text, size_t len);
    bool PushAndFlush(WindowsEventLogBackend* backend, Level level, const char* text, size_t len);
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
