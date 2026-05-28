#pragma once

#include <condition_variable>
#include <mutex>
#include <vector>
#include <stdint.h>
#include <thread>

#include <Logme/Backend/FileBackend.h>
#include <Logme/Buffer/BufferCounters.h>

namespace Logme
{
  struct FileManagerCounters
  {
    std::uint64_t AddBackendCalls = 0;
    std::uint64_t NotifyCalls = 0;
    std::uint64_t RescheduleSignals = 0;
    std::uint64_t TestFileInUseCalls = 0;
    std::uint64_t ManagementLoops = 0;
    std::uint64_t WaitCalls = 0;
    std::uint64_t WaitWakeups = 0;
    std::uint64_t WorkerRuns = 0;
    std::uint64_t BackendShutdowns = 0;
    std::uint64_t BackendRemovals = 0;
    std::uint64_t BackendScanPasses = 0;
    std::uint64_t BackendScanItems = 0;
    std::uint64_t BackendScanIdleItems = 0;
    std::uint64_t BackendScanRightNowItems = 0;
    std::uint64_t BackendScanScheduledItems = 0;
    std::uint64_t BackendScanNoWork = 0;
    std::uint64_t BackendScanSelectedRightNow = 0;
    std::uint64_t BackendScanSelectedScheduled = 0;
    std::uint64_t TimedWaitCalls = 0;
    std::uint64_t IdleWaitCalls = 0;
    std::uint64_t DueScheduledRuns = 0;
    std::uint64_t PastDueScheduledRuns = 0;
    std::uint64_t NotifyRightNowCalls = 0;
    std::uint64_t NotifyEarlierCalls = 0;
    std::uint64_t NotifyIgnoredCalls = 0;
  };

  class FileManager
  {
    bool StopRequested;
    bool Reschedule;
    std::thread ManagerThread;

    std::mutex Lock;
    std::condition_variable CV;
    std::vector<FileBackendPtr> Backends;
    FileBackend* ActiveBackendsNext;
    FileBackend* ActiveBackendsPrev;

    uint64_t CurrentEarliestTime;

#ifdef _WIN32
    unsigned ThreadID;
#endif

  public:
    FileManager();
    ~FileManager();

    void AddBackend(const FileBackendPtr& backend);
    void Notify(FileBackend* backend, uint64_t when);

    bool Stopping() const;
    void SetStopping();

    bool TestFileInUse(const std::string& file);
    static FileManagerCounters GetCounters();

  private:
    void ActivateBackend(FileBackend* backend);
    void DeactivateBackend(FileBackend* backend);
    bool RemoveBackendLocked(const FileBackendPtr& backend);
    void ManagementThread();
  };
}
