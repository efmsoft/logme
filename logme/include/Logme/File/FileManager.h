#pragma once

#include <condition_variable>
#include <mutex>
#include <stdint.h>
#include <thread>
#include <vector>

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
    std::uint64_t ActiveQueueStaleItems = 0;
    std::uint64_t HeadRightNowChecks = 0;
    std::uint64_t HeadScheduledChecks = 0;
    std::uint64_t TimedWaitCalls = 0;
    std::uint64_t IdleWaitCalls = 0;
    std::uint64_t DueScheduledRuns = 0;
    std::uint64_t PastDueScheduledRuns = 0;
    std::uint64_t NotifyRightNowCalls = 0;
    std::uint64_t NotifyEarlierCalls = 0;
    std::uint64_t NotifyIgnoredCalls = 0;
    std::uint64_t ActiveQueuePushFront = 0;
    std::uint64_t ActiveQueuePushBack = 0;
    std::uint64_t ActiveQueueSortedInsert = 0;
    std::uint64_t ActiveQueueRemove = 0;
    std::uint64_t ActiveQueueReorder = 0;
    std::uint64_t ActiveQueueEmpty = 0;
    std::uint64_t CurrentActiveDepth = 0;
    std::uint64_t MaxActiveDepth = 0;
    std::uint64_t HeadRightNowSelected = 0;
    std::uint64_t HeadScheduledSelected = 0;
    std::uint64_t TimedWaitTimeouts = 0;
    std::uint64_t TimedWaitReschedules = 0;
    std::uint64_t IdleWaitReschedules = 0;
    std::uint64_t ScheduledEarlyRuns = 0;
    std::uint64_t ScheduledLateRuns = 0;
    std::uint64_t TotalScheduledDelayMs = 0;
    std::uint64_t MaxScheduledDelayMs = 0;
    std::uint64_t DataBufferCacheHits = 0;
    std::uint64_t DataBufferCacheMisses = 0;
    std::uint64_t DataBufferCacheReturns = 0;
    std::uint64_t DataBufferCacheDrops = 0;
    std::uint64_t DataBufferCacheDepth = 0;
    std::uint64_t DataBufferCacheMaxDepth = 0;
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
    std::vector<DataBufferPtr> DataBufferCache;

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

    DataBufferPtr TakeDataBuffer(
      MemoryUsageTracker* memoryTracker
      , std::size_t capacity
    );
    bool ReturnDataBuffer(DataBufferPtr buffer, std::size_t cacheLimit);
    void TrimDataBufferCache(std::size_t cacheLimit);

  private:
    void LinkBackendFront(FileBackend* backend);
    void LinkBackendBack(FileBackend* backend);
    void LinkBackendBefore(FileBackend* backend, FileBackend* before);
    void ActivateBackend(FileBackend* backend, uint64_t when);
    void DeactivateBackend(FileBackend* backend);
    bool RemoveBackendLocked(const FileBackendPtr& backend);
    void ManagementThread();
  };
}
