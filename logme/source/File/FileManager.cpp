#include <algorithm>
#include <atomic>
#include <cstdio>

#include <Logme/Backend/FileBackend.h>
#include <Logme/File/FileManager.h>
#include <Logme/Time/datetime.h> 
#include <Logme/Utils.h> 

using namespace Logme;

namespace
{
  std::atomic<std::uint64_t> GlobalAddBackendCalls(0);
  std::atomic<std::uint64_t> GlobalNotifyCalls(0);
  std::atomic<std::uint64_t> GlobalRescheduleSignals(0);
  std::atomic<std::uint64_t> GlobalTestFileInUseCalls(0);
  std::atomic<std::uint64_t> GlobalManagementLoops(0);
  std::atomic<std::uint64_t> GlobalWaitCalls(0);
  std::atomic<std::uint64_t> GlobalWaitWakeups(0);
  std::atomic<std::uint64_t> GlobalWorkerRuns(0);
  std::atomic<std::uint64_t> GlobalBackendShutdowns(0);
  std::atomic<std::uint64_t> GlobalBackendRemovals(0);
  std::atomic<std::uint64_t> GlobalActiveQueueStaleItems(0);
  std::atomic<std::uint64_t> GlobalHeadRightNowChecks(0);
  std::atomic<std::uint64_t> GlobalHeadScheduledChecks(0);
  std::atomic<std::uint64_t> GlobalTimedWaitCalls(0);
  std::atomic<std::uint64_t> GlobalIdleWaitCalls(0);
  std::atomic<std::uint64_t> GlobalDueScheduledRuns(0);
  std::atomic<std::uint64_t> GlobalPastDueScheduledRuns(0);
  std::atomic<std::uint64_t> GlobalNotifyRightNowCalls(0);
  std::atomic<std::uint64_t> GlobalNotifyEarlierCalls(0);
  std::atomic<std::uint64_t> GlobalNotifyIgnoredCalls(0);
  std::atomic<std::uint64_t> GlobalActiveQueuePushFront(0);
  std::atomic<std::uint64_t> GlobalActiveQueuePushBack(0);
  std::atomic<std::uint64_t> GlobalActiveQueueSortedInsert(0);
  std::atomic<std::uint64_t> GlobalActiveQueueRemove(0);
  std::atomic<std::uint64_t> GlobalActiveQueueReorder(0);
  std::atomic<std::uint64_t> GlobalActiveQueueEmpty(0);
  std::atomic<std::uint64_t> GlobalCurrentActiveDepth(0);
  std::atomic<std::uint64_t> GlobalMaxActiveDepth(0);
  std::atomic<std::uint64_t> GlobalHeadRightNowSelected(0);
  std::atomic<std::uint64_t> GlobalHeadScheduledSelected(0);
  std::atomic<std::uint64_t> GlobalTimedWaitTimeouts(0);
  std::atomic<std::uint64_t> GlobalTimedWaitReschedules(0);
  std::atomic<std::uint64_t> GlobalIdleWaitReschedules(0);
  std::atomic<std::uint64_t> GlobalScheduledEarlyRuns(0);
  std::atomic<std::uint64_t> GlobalScheduledLateRuns(0);
  std::atomic<std::uint64_t> GlobalTotalScheduledDelayMs(0);
  std::atomic<std::uint64_t> GlobalMaxScheduledDelayMs(0);
  std::atomic<std::uint64_t> GlobalDataBufferCacheHits(0);
  std::atomic<std::uint64_t> GlobalDataBufferCacheMisses(0);
  std::atomic<std::uint64_t> GlobalDataBufferCacheReturns(0);
  std::atomic<std::uint64_t> GlobalDataBufferCacheDrops(0);
  std::atomic<std::uint64_t> GlobalDataBufferCacheDepth(0);
  std::atomic<std::uint64_t> GlobalDataBufferCacheMaxDepth(0);

  constexpr std::uint64_t SCHEDULED_RUN_GAP_MS = 20;

#if FILE_ENABLE_COUNTERS
  void UpdateMaxCounter(
    std::atomic<std::uint64_t>& counter
    , std::uint64_t value
  )
  {
    std::uint64_t current = counter.load(std::memory_order_relaxed);
    while (current < value &&
      !counter.compare_exchange_weak(
        current
        , value
        , std::memory_order_relaxed
        , std::memory_order_relaxed
      ))
    {
    }
  }

  void IncrementActiveDepth()
  {
    std::uint64_t depth = GlobalCurrentActiveDepth.fetch_add(
      1
      , std::memory_order_relaxed
    ) + 1;
    UpdateMaxCounter(GlobalMaxActiveDepth, depth);
  }

  void DecrementActiveDepth()
  {
    GlobalCurrentActiveDepth.fetch_sub(1, std::memory_order_relaxed);
  }
#endif
}

FileManagerCounters FileManager::GetCounters()
{
  FileManagerCounters out;
  out.AddBackendCalls = GlobalAddBackendCalls.load(std::memory_order_relaxed);
  out.NotifyCalls = GlobalNotifyCalls.load(std::memory_order_relaxed);
  out.RescheduleSignals = GlobalRescheduleSignals.load(std::memory_order_relaxed);
  out.TestFileInUseCalls = GlobalTestFileInUseCalls.load(std::memory_order_relaxed);
  out.ManagementLoops = GlobalManagementLoops.load(std::memory_order_relaxed);
  out.WaitCalls = GlobalWaitCalls.load(std::memory_order_relaxed);
  out.WaitWakeups = GlobalWaitWakeups.load(std::memory_order_relaxed);
  out.WorkerRuns = GlobalWorkerRuns.load(std::memory_order_relaxed);
  out.BackendShutdowns = GlobalBackendShutdowns.load(std::memory_order_relaxed);
  out.BackendRemovals = GlobalBackendRemovals.load(std::memory_order_relaxed);
  out.ActiveQueueStaleItems = GlobalActiveQueueStaleItems.load(std::memory_order_relaxed);
  out.HeadRightNowChecks =
    GlobalHeadRightNowChecks.load(std::memory_order_relaxed);
  out.HeadScheduledChecks =
    GlobalHeadScheduledChecks.load(std::memory_order_relaxed);
  out.TimedWaitCalls = GlobalTimedWaitCalls.load(std::memory_order_relaxed);
  out.IdleWaitCalls = GlobalIdleWaitCalls.load(std::memory_order_relaxed);
  out.DueScheduledRuns = GlobalDueScheduledRuns.load(std::memory_order_relaxed);
  out.PastDueScheduledRuns =
    GlobalPastDueScheduledRuns.load(std::memory_order_relaxed);
  out.NotifyRightNowCalls = GlobalNotifyRightNowCalls.load(std::memory_order_relaxed);
  out.NotifyEarlierCalls = GlobalNotifyEarlierCalls.load(std::memory_order_relaxed);
  out.NotifyIgnoredCalls = GlobalNotifyIgnoredCalls.load(std::memory_order_relaxed);
  out.ActiveQueuePushFront =
    GlobalActiveQueuePushFront.load(std::memory_order_relaxed);
  out.ActiveQueuePushBack =
    GlobalActiveQueuePushBack.load(std::memory_order_relaxed);
  out.ActiveQueueSortedInsert =
    GlobalActiveQueueSortedInsert.load(std::memory_order_relaxed);
  out.ActiveQueueRemove =
    GlobalActiveQueueRemove.load(std::memory_order_relaxed);
  out.ActiveQueueReorder =
    GlobalActiveQueueReorder.load(std::memory_order_relaxed);
  out.ActiveQueueEmpty =
    GlobalActiveQueueEmpty.load(std::memory_order_relaxed);
  out.CurrentActiveDepth =
    GlobalCurrentActiveDepth.load(std::memory_order_relaxed);
  out.MaxActiveDepth = GlobalMaxActiveDepth.load(std::memory_order_relaxed);
  out.HeadRightNowSelected =
    GlobalHeadRightNowSelected.load(std::memory_order_relaxed);
  out.HeadScheduledSelected =
    GlobalHeadScheduledSelected.load(std::memory_order_relaxed);
  out.TimedWaitTimeouts = GlobalTimedWaitTimeouts.load(std::memory_order_relaxed);
  out.TimedWaitReschedules =
    GlobalTimedWaitReschedules.load(std::memory_order_relaxed);
  out.IdleWaitReschedules =
    GlobalIdleWaitReschedules.load(std::memory_order_relaxed);
  out.ScheduledEarlyRuns = GlobalScheduledEarlyRuns.load(std::memory_order_relaxed);
  out.ScheduledLateRuns = GlobalScheduledLateRuns.load(std::memory_order_relaxed);
  out.TotalScheduledDelayMs =
    GlobalTotalScheduledDelayMs.load(std::memory_order_relaxed);
  out.MaxScheduledDelayMs =
    GlobalMaxScheduledDelayMs.load(std::memory_order_relaxed);
  out.DataBufferCacheHits =
    GlobalDataBufferCacheHits.load(std::memory_order_relaxed);
  out.DataBufferCacheMisses =
    GlobalDataBufferCacheMisses.load(std::memory_order_relaxed);
  out.DataBufferCacheReturns =
    GlobalDataBufferCacheReturns.load(std::memory_order_relaxed);
  out.DataBufferCacheDrops =
    GlobalDataBufferCacheDrops.load(std::memory_order_relaxed);
  out.DataBufferCacheDepth =
    GlobalDataBufferCacheDepth.load(std::memory_order_relaxed);
  out.DataBufferCacheMaxDepth =
    GlobalDataBufferCacheMaxDepth.load(std::memory_order_relaxed);
  return out;
}


DataBufferPtr FileManager::TakeDataBuffer(
  MemoryUsageTracker* memoryTracker
  , std::size_t capacity
)
{
  std::lock_guard guard(Lock);

  if (DataBufferCache.empty())
  {
    FILE_CNT(GlobalDataBufferCacheMisses.fetch_add(
      1
      , std::memory_order_relaxed
    ));
    return nullptr;
  }

  DataBufferPtr buffer = std::move(DataBufferCache.back());
  DataBufferCache.pop_back();
  FILE_CNT(GlobalDataBufferCacheDepth.fetch_sub(
    1
    , std::memory_order_relaxed
  ));

  if (!buffer || buffer->Capacity() != capacity)
  {
    FILE_CNT(GlobalDataBufferCacheMisses.fetch_add(
      1
      , std::memory_order_relaxed
    ));
    return nullptr;
  }

  buffer->AttachMemoryTracker(memoryTracker);
  buffer->Reset();

  FILE_CNT(GlobalDataBufferCacheHits.fetch_add(
    1
    , std::memory_order_relaxed
  ));
  return buffer;
}

bool FileManager::ReturnDataBuffer(
  DataBufferPtr buffer
  , std::size_t cacheLimit
)
{
  if (!buffer || cacheLimit == 0)
  {
    FILE_CNT(GlobalDataBufferCacheDrops.fetch_add(
      1
      , std::memory_order_relaxed
    ));
    return false;
  }

  std::lock_guard guard(Lock);

  if (DataBufferCache.size() >= cacheLimit)
  {
    FILE_CNT(GlobalDataBufferCacheDrops.fetch_add(
      1
      , std::memory_order_relaxed
    ));
    return false;
  }

  buffer->Reset();
  buffer->DetachMemoryTracker();
  DataBufferCache.push_back(std::move(buffer));
  std::uint64_t depth = GlobalDataBufferCacheDepth.fetch_add(
    1
    , std::memory_order_relaxed
  ) + 1;

  FILE_CNT(GlobalDataBufferCacheReturns.fetch_add(
    1
    , std::memory_order_relaxed
  ));
  FILE_CNT(UpdateMaxCounter(GlobalDataBufferCacheMaxDepth, depth));
  return true;
}

void FileManager::TrimDataBufferCache(std::size_t cacheLimit)
{
  std::lock_guard guard(Lock);

  while (DataBufferCache.size() > cacheLimit)
  {
    DataBufferCache.pop_back();
    FILE_CNT(GlobalDataBufferCacheDrops.fetch_add(
      1
      , std::memory_order_relaxed
    ));
    FILE_CNT(GlobalDataBufferCacheDepth.fetch_sub(
      1
      , std::memory_order_relaxed
    ));
  }
}

FileManager::FileManager()
  : StopRequested(false)
  , Reschedule(false)
  , ActiveBackendsNext(nullptr)
  , ActiveBackendsPrev(nullptr)
  , CurrentEarliestTime(0)
#ifdef _WIN32
  , ThreadID(0)
#endif
{
}

FileManager::~FileManager()
{
  SetStopping();

  if (ManagerThread.joinable()) 
    ManagerThread.join();

  for (const auto& backend : Backends)
  {
    DeactivateBackend(backend.get());
    backend->Registered.store(false, std::memory_order_relaxed);
    backend->FlushTime.store(0, std::memory_order_relaxed);
    FILE_CNT(GlobalBackendShutdowns.fetch_add(1, std::memory_order_relaxed));
    backend->OnShutdown();
  }

#if FILE_ENABLE_COUNTERS || FILE_ENABLE_WRITE_READY_COUNTERS || FILE_ENABLE_FLUSH_SOURCE_COUNTERS
  {
    const FileManagerCounters mc = GetCounters();
    const FileBackendCounters bc = FileBackend::GetCounters();

    printf(
      "[FileManager] "
      "AddBackendCalls=%llu "
      "NotifyCalls=%llu "
      "RescheduleSignals=%llu "
      "TestFileInUseCalls=%llu "
      "ManagementLoops=%llu "
      "WaitCalls=%llu "
      "WaitWakeups=%llu "
      "WorkerRuns=%llu "
      "BackendShutdowns=%llu "
      "BackendRemovals=%llu "
      "ActiveQueueStaleItems=%llu "
      "HeadRightNowChecks=%llu "
      "HeadScheduledChecks=%llu "
      "TimedWaitCalls=%llu "
      "IdleWaitCalls=%llu "
      "DueScheduledRuns=%llu "
      "PastDueScheduledRuns=%llu "
      "NotifyRightNowCalls=%llu "
      "NotifyEarlierCalls=%llu "
      "NotifyIgnoredCalls=%llu "
      "ActiveQueuePushFront=%llu "
      "ActiveQueuePushBack=%llu "
      "ActiveQueueSortedInsert=%llu "
      "ActiveQueueRemove=%llu "
      "ActiveQueueReorder=%llu "
      "ActiveQueueEmpty=%llu "
      "CurrentActiveDepth=%llu "
      "MaxActiveDepth=%llu "
      "HeadRightNowSelected=%llu "
      "HeadScheduledSelected=%llu "
      "TimedWaitTimeouts=%llu "
      "TimedWaitReschedules=%llu "
      "IdleWaitReschedules=%llu "
      "ScheduledEarlyRuns=%llu "
      "ScheduledLateRuns=%llu "
      "TotalScheduledDelayMs=%llu "
      "MaxScheduledDelayMs=%llu "
      "DataBufferCacheHits=%llu "
      "DataBufferCacheMisses=%llu "
      "DataBufferCacheReturns=%llu "
      "DataBufferCacheDrops=%llu "
      "DataBufferCacheDepth=%llu "
      "DataBufferCacheMaxDepth=%llu\n"
      , (unsigned long long)mc.AddBackendCalls
      , (unsigned long long)mc.NotifyCalls
      , (unsigned long long)mc.RescheduleSignals
      , (unsigned long long)mc.TestFileInUseCalls
      , (unsigned long long)mc.ManagementLoops
      , (unsigned long long)mc.WaitCalls
      , (unsigned long long)mc.WaitWakeups
      , (unsigned long long)mc.WorkerRuns
      , (unsigned long long)mc.BackendShutdowns
      , (unsigned long long)mc.BackendRemovals
      , (unsigned long long)mc.ActiveQueueStaleItems
      , (unsigned long long)mc.HeadRightNowChecks
      , (unsigned long long)mc.HeadScheduledChecks
      , (unsigned long long)mc.TimedWaitCalls
      , (unsigned long long)mc.IdleWaitCalls
      , (unsigned long long)mc.DueScheduledRuns
      , (unsigned long long)mc.PastDueScheduledRuns
      , (unsigned long long)mc.NotifyRightNowCalls
      , (unsigned long long)mc.NotifyEarlierCalls
      , (unsigned long long)mc.NotifyIgnoredCalls
      , (unsigned long long)mc.ActiveQueuePushFront
      , (unsigned long long)mc.ActiveQueuePushBack
      , (unsigned long long)mc.ActiveQueueSortedInsert
      , (unsigned long long)mc.ActiveQueueRemove
      , (unsigned long long)mc.ActiveQueueReorder
      , (unsigned long long)mc.ActiveQueueEmpty
      , (unsigned long long)mc.CurrentActiveDepth
      , (unsigned long long)mc.MaxActiveDepth
      , (unsigned long long)mc.HeadRightNowSelected
      , (unsigned long long)mc.HeadScheduledSelected
      , (unsigned long long)mc.TimedWaitTimeouts
      , (unsigned long long)mc.TimedWaitReschedules
      , (unsigned long long)mc.IdleWaitReschedules
      , (unsigned long long)mc.ScheduledEarlyRuns
      , (unsigned long long)mc.ScheduledLateRuns
      , (unsigned long long)mc.TotalScheduledDelayMs
      , (unsigned long long)mc.MaxScheduledDelayMs
      , (unsigned long long)mc.DataBufferCacheHits
      , (unsigned long long)mc.DataBufferCacheMisses
      , (unsigned long long)mc.DataBufferCacheReturns
      , (unsigned long long)mc.DataBufferCacheDrops
      , (unsigned long long)mc.DataBufferCacheDepth
      , (unsigned long long)mc.DataBufferCacheMaxDepth
    );

    printf(
      "[FileBackend] "
      "DisplayCalls=%llu "
      "AppendCalls=%llu "
      "InputBytes=%llu "
      "OutputBytes=%llu "
      "FlushRequests=%llu "
      "ImmediateFlushRequests=%llu "
      "ScheduledFlushRequests=%llu "
      "FlushAccepted=%llu "
      "ImmediateFlushAccepted=%llu "
      "ScheduledFlushAccepted=%llu "
      "FlushRejected=%llu "
      "ImmediateFlushRejected=%llu "
      "ScheduledFlushRejected=%llu "
      "FlushWaitCalls=%llu "
      "WorkerRuns=%llu "
      "WorkerImmediateRuns=%llu "
      "WorkerTimedRuns=%llu "
      "WorkerShutdownRuns=%llu "
      "WriteReadyCalls=%llu "
      "WriteBatches=%llu "
      "WriteBatchBytes=%llu "
      "WriteBatchMaxBytes=%llu "
      "WrittenBuffers=%llu "
      "WrittenBytes=%llu "
      "WriteErrors=%llu "
      "CreateLogCalls=%llu "
      "CreateLogFailures=%llu "
      "ChangePartCalls=%llu "
      "ChangePartFailures=%llu "
      "ShutdownCalls=%llu "
      "Queue.Appends=%llu "
      "Queue.AppendBytes=%llu "
      "Queue.EnqueuedBuffers=%llu "
      "Queue.WrittenBuffers=%llu "
      "Queue.WrittenBytes=%llu "
      "Queue.AllocatedBuffers=%llu "
      "Queue.DeletedBuffers=%llu "
      "Queue.DroppedAppends=%llu "
      "Queue.DroppedBytes=%llu "
      "Queue.WriteErrors=%llu "
      "Queue.SignalsSent=%llu\n"
      , (unsigned long long)bc.DisplayCalls
      , (unsigned long long)bc.AppendCalls
      , (unsigned long long)bc.InputBytes
      , (unsigned long long)bc.OutputBytes
      , (unsigned long long)bc.FlushRequests
      , (unsigned long long)bc.ImmediateFlushRequests
      , (unsigned long long)bc.ScheduledFlushRequests
      , (unsigned long long)bc.FlushAccepted
      , (unsigned long long)bc.ImmediateFlushAccepted
      , (unsigned long long)bc.ScheduledFlushAccepted
      , (unsigned long long)bc.FlushRejected
      , (unsigned long long)bc.ImmediateFlushRejected
      , (unsigned long long)bc.ScheduledFlushRejected
      , (unsigned long long)bc.FlushWaitCalls
      , (unsigned long long)bc.WorkerRuns
      , (unsigned long long)bc.WorkerImmediateRuns
      , (unsigned long long)bc.WorkerTimedRuns
      , (unsigned long long)bc.WorkerShutdownRuns
      , (unsigned long long)bc.WriteReadyCalls
      , (unsigned long long)bc.WriteBatches
      , (unsigned long long)bc.WriteBatchBytes
      , (unsigned long long)bc.WriteBatchMaxBytes
      , (unsigned long long)bc.WrittenBuffers
      , (unsigned long long)bc.WrittenBytes
      , (unsigned long long)bc.WriteErrors
      , (unsigned long long)bc.CreateLogCalls
      , (unsigned long long)bc.CreateLogFailures
      , (unsigned long long)bc.ChangePartCalls
      , (unsigned long long)bc.ChangePartFailures
      , (unsigned long long)bc.ShutdownCalls
      , (unsigned long long)bc.Queue.Appends
      , (unsigned long long)bc.Queue.AppendBytes
      , (unsigned long long)bc.Queue.EnqueuedBuffers
      , (unsigned long long)bc.Queue.WrittenBuffers
      , (unsigned long long)bc.Queue.WrittenBytes
      , (unsigned long long)bc.Queue.AllocatedBuffers
      , (unsigned long long)bc.Queue.DeletedBuffers
      , (unsigned long long)bc.Queue.DroppedAppends
      , (unsigned long long)bc.Queue.DroppedBytes
      , (unsigned long long)bc.Queue.WriteErrors
      , (unsigned long long)bc.Queue.SignalsSent
    );

#if FILE_ENABLE_WRITE_READY_COUNTERS
    printf(
      "[FileBackend.WriteReadyData] "
      "EmptyCalls=%llu "
      "RawCalls=%llu "
      "RawBytes=%llu "
      "RawMaxBytes=%llu "
      "MaxBuffers=%llu "
      "MaxBytes=%llu "
      "WorkerWriteLoopIterations=%llu "
      "WorkerWriteLoopMaxIterations=%llu "
      "WorkerPublishCurrentCalls=%llu "
      "WorkerPublishCurrentSuccess=%llu "
      "WorkerBreaks=%llu\n"
      , (unsigned long long)bc.WriteReadyEmptyCalls
      , (unsigned long long)bc.WriteReadyRawCalls
      , (unsigned long long)bc.WriteReadyRawBytes
      , (unsigned long long)bc.WriteReadyRawMaxBytes
      , (unsigned long long)bc.WriteReadyMaxBuffers
      , (unsigned long long)bc.WriteReadyMaxBytes
      , (unsigned long long)bc.WorkerWriteLoopIterations
      , (unsigned long long)bc.WorkerWriteLoopMaxIterations
      , (unsigned long long)bc.WorkerPublishCurrentCalls
      , (unsigned long long)bc.WorkerPublishCurrentSuccess
      , (unsigned long long)bc.WorkerBreaks
    );
#endif

#if FILE_ENABLE_FLUSH_SOURCE_COUNTERS
    printf(
      "[FileBackend.FlushSource] "
      "PublishFromFlushCalls=%llu "
      "PublishFromFlushSuccess=%llu "
      "PublishFromWorkerImmediateCalls=%llu "
      "PublishFromWorkerImmediateSuccess=%llu "
      "PublishFromWorkerShutdownCalls=%llu "
      "PublishFromWorkerShutdownSuccess=%llu "
      "PublishFromOnShutdownCalls=%llu "
      "PublishFromOnShutdownSuccess=%llu "
      "RequestRightNowFromFlush=%llu "
      "RequestRightNowFromFreeze=%llu "
      "RequestRightNowFromPressureBuffers=%llu "
      "RequestRightNowFromPressureBytes=%llu "
      "RequestRightNowFromPressureBoth=%llu "
      "RequestScheduledFromFirstData=%llu "
      "PressureByBuffers=%llu "
      "PressureByBytes=%llu "
      "PressureByBoth=%llu "
      "PublishCurrentQueuedBytes=%llu "
      "PublishCurrentMaxQueuedBytes=%llu "
      "PublishCurrentAgeMs=%llu "
      "PublishCurrentMaxAgeMs=%llu\n"
      , (unsigned long long)bc.PublishFromFlushCalls
      , (unsigned long long)bc.PublishFromFlushSuccess
      , (unsigned long long)bc.PublishFromWorkerImmediateCalls
      , (unsigned long long)bc.PublishFromWorkerImmediateSuccess
      , (unsigned long long)bc.PublishFromWorkerShutdownCalls
      , (unsigned long long)bc.PublishFromWorkerShutdownSuccess
      , (unsigned long long)bc.PublishFromOnShutdownCalls
      , (unsigned long long)bc.PublishFromOnShutdownSuccess
      , (unsigned long long)bc.RequestRightNowFromFlush
      , (unsigned long long)bc.RequestRightNowFromFreeze
      , (unsigned long long)bc.RequestRightNowFromPressureBuffers
      , (unsigned long long)bc.RequestRightNowFromPressureBytes
      , (unsigned long long)bc.RequestRightNowFromPressureBoth
      , (unsigned long long)bc.RequestScheduledFromFirstData
      , (unsigned long long)bc.PressureByBuffers
      , (unsigned long long)bc.PressureByBytes
      , (unsigned long long)bc.PressureByBoth
      , (unsigned long long)bc.PublishCurrentQueuedBytes
      , (unsigned long long)bc.PublishCurrentMaxQueuedBytes
      , (unsigned long long)bc.PublishCurrentAgeMs
      , (unsigned long long)bc.PublishCurrentMaxAgeMs
    );
#endif
  }
#endif
}

void FileManager::AddBackend(const FileBackendPtr& backend)
{
  FILE_CNT(GlobalAddBackendCalls.fetch_add(1, std::memory_order_relaxed));
  std::lock_guard lock(Lock);
  if (std::find(Backends.begin(), Backends.end(), backend) == Backends.end())
    Backends.push_back(backend);
  
  if (!ManagerThread.joinable())
    ManagerThread = std::thread(&FileManager::ManagementThread, this);
}

bool FileManager::Stopping() const
{
  return StopRequested;
}

#ifdef _WIN32
#include <windows.h>

static bool ThreadExists(DWORD threadId)
{
  HANDLE hThread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, threadId);
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

void FileManager::SetStopping()
{
#ifdef _WIN32
  std::vector<FileBackendPtr> shutdownBackends;
#endif

  if (true)
  {
    std::lock_guard<std::mutex> lock(Lock);
    
    StopRequested = true;
    Reschedule = true;

#ifdef _WIN32
    if (ThreadExists(ThreadID) == false)
    {
      // In Windows, the handling of ExitProcess is implemented in a very 
      // specific way. If our library is in a DLL, then all threads created 
      // by the library are simply terminated by the system during ExitProcess 
      // handling, without sending any notifications. Since this function is 
      // called from the Logger destructor, it's very likely that any thread 
      // we created no longer exists at that point. Therefore, we simply remove 
      // all backends from the list to drop our references to FileBackend, 
      // allowing them to be destroyed properly.
      for (const auto& backend : Backends)
      {
        DeactivateBackend(backend.get());
        backend->Registered.store(false, std::memory_order_relaxed);
        backend->FlushTime.store(0, std::memory_order_relaxed);
      }

      shutdownBackends = std::move(Backends);
      ActiveBackendsNext = nullptr;
      ActiveBackendsPrev = nullptr;
    }
#endif
  }

  CV.notify_all();

#ifdef _WIN32
  for (const auto& backend : shutdownBackends)
    backend->OnShutdown();
#endif
}

void FileManager::LinkBackendFront(FileBackend* backend)
{
  backend->ActivePrev = nullptr;
  backend->ActiveNext = ActiveBackendsNext;

  if (ActiveBackendsNext != nullptr)
    ActiveBackendsNext->ActivePrev = backend;
  else
    ActiveBackendsPrev = backend;

  ActiveBackendsNext = backend;
  backend->ActiveLinked = true;
  FILE_CNT(GlobalActiveQueuePushFront.fetch_add(1, std::memory_order_relaxed));
  FILE_CNT(IncrementActiveDepth());
}

void FileManager::LinkBackendBack(FileBackend* backend)
{
  backend->ActiveNext = nullptr;
  backend->ActivePrev = ActiveBackendsPrev;

  if (ActiveBackendsPrev != nullptr)
    ActiveBackendsPrev->ActiveNext = backend;
  else
    ActiveBackendsNext = backend;

  ActiveBackendsPrev = backend;
  backend->ActiveLinked = true;
  FILE_CNT(GlobalActiveQueuePushBack.fetch_add(1, std::memory_order_relaxed));
  FILE_CNT(IncrementActiveDepth());
}

void FileManager::LinkBackendBefore(FileBackend* backend, FileBackend* before)
{
  if (before == nullptr)
  {
    LinkBackendBack(backend);
    return;
  }

  backend->ActivePrev = before->ActivePrev;
  backend->ActiveNext = before;

  if (before->ActivePrev != nullptr)
    before->ActivePrev->ActiveNext = backend;
  else
    ActiveBackendsNext = backend;

  before->ActivePrev = backend;
  backend->ActiveLinked = true;
  FILE_CNT(GlobalActiveQueueSortedInsert.fetch_add(1, std::memory_order_relaxed));
  FILE_CNT(IncrementActiveDepth());
}

void FileManager::ActivateBackend(FileBackend* backend, uint64_t when)
{
  if (backend == nullptr || when == 0)
    return;

  if (backend->ActiveLinked)
    FILE_CNT(GlobalActiveQueueReorder.fetch_add(1, std::memory_order_relaxed));

  DeactivateBackend(backend);

  if (when == FileBackend::RIGHT_NOW)
  {
    LinkBackendFront(backend);
    return;
  }

  if (ActiveBackendsPrev == nullptr)
  {
    LinkBackendBack(backend);
    return;
  }

  uint64_t lastTime = ActiveBackendsPrev->GetFlushTime();
  if (lastTime != FileBackend::RIGHT_NOW && lastTime <= when)
  {
    LinkBackendBack(backend);
    return;
  }

  for (FileBackend* item = ActiveBackendsNext; item != nullptr; item = item->ActiveNext)
  {
    uint64_t itemTime = item->GetFlushTime();
    if (itemTime != FileBackend::RIGHT_NOW && itemTime > when)
    {
      LinkBackendBefore(backend, item);
      return;
    }
  }

  LinkBackendBack(backend);
}

void FileManager::DeactivateBackend(FileBackend* backend)
{
  if (backend == nullptr || !backend->ActiveLinked)
    return;

  if (backend->ActivePrev != nullptr)
    backend->ActivePrev->ActiveNext = backend->ActiveNext;
  else
    ActiveBackendsNext = backend->ActiveNext;

  if (backend->ActiveNext != nullptr)
    backend->ActiveNext->ActivePrev = backend->ActivePrev;
  else
    ActiveBackendsPrev = backend->ActivePrev;

  backend->ActiveNext = nullptr;
  backend->ActivePrev = nullptr;
  backend->ActiveLinked = false;
  FILE_CNT(GlobalActiveQueueRemove.fetch_add(1, std::memory_order_relaxed));
  FILE_CNT(DecrementActiveDepth());
}

bool FileManager::RemoveBackendLocked(const FileBackendPtr& backend)
{
  if (!backend)
    return false;

  DeactivateBackend(backend.get());
  backend->Registered.store(false, std::memory_order_relaxed);
  backend->FlushTime.store(0, std::memory_order_relaxed);

  auto backendPos = std::find(Backends.begin(), Backends.end(), backend);
  if (backendPos == Backends.end())
    return false;

  Backends.erase(backendPos);
  FILE_CNT(GlobalBackendRemovals.fetch_add(1, std::memory_order_relaxed));
  return true;
}

void FileManager::Notify(FileBackend* backend, uint64_t when)
{
  FILE_CNT(GlobalNotifyCalls.fetch_add(1, std::memory_order_relaxed));
  std::lock_guard lock(Lock);

  if (backend == nullptr || !backend->Registered.load(std::memory_order_relaxed))
  {
    FILE_CNT(GlobalNotifyIgnoredCalls.fetch_add(1, std::memory_order_relaxed));
    return;
  }

  ActivateBackend(backend, when);
  
  if (ActiveBackendsNext == backend || when == FileBackend::RIGHT_NOW || when < CurrentEarliestTime)
  {
    if (when == FileBackend::RIGHT_NOW)
      FILE_CNT(GlobalNotifyRightNowCalls.fetch_add(1, std::memory_order_relaxed));
    else
      FILE_CNT(GlobalNotifyEarlierCalls.fetch_add(1, std::memory_order_relaxed));

    Reschedule = true;
    FILE_CNT(GlobalRescheduleSignals.fetch_add(1, std::memory_order_relaxed));
    CV.notify_all();
  }
  else
    FILE_CNT(GlobalNotifyIgnoredCalls.fetch_add(1, std::memory_order_relaxed));
}

bool FileManager::TestFileInUse(const std::string& file)
{
  FILE_CNT(GlobalTestFileInUseCalls.fetch_add(1, std::memory_order_relaxed));
  std::lock_guard lock(Lock);

  for (const auto& backend : Backends)
    if (backend->TestFileInUse(file))
      return true;
  
  return false;
}

void FileManager::ManagementThread()
{
  RenameThread(uint64_t(-1), "FileManager::ManagementThread");

#ifdef _WIN32
  ThreadID = (unsigned)GetCurrentThreadId();
#endif

  std::unique_lock lock(Lock);

  while (StopRequested == false)
  {
    FILE_CNT(GlobalManagementLoops.fetch_add(1, std::memory_order_relaxed));

    FileBackendPtr nextToRun;
    FileBackend* nextToRunRaw = ActiveBackendsNext;

    while (nextToRunRaw != nullptr && nextToRunRaw->GetFlushTime() == 0)
    {
      FILE_CNT(GlobalActiveQueueStaleItems.fetch_add(1, std::memory_order_relaxed));
      DeactivateBackend(nextToRunRaw);
      nextToRunRaw = ActiveBackendsNext;
    }

    if (nextToRunRaw == nullptr)
    {
      FILE_CNT(GlobalActiveQueueEmpty.fetch_add(1, std::memory_order_relaxed));
      CurrentEarliestTime = UINT64_MAX;

      FILE_CNT(GlobalWaitCalls.fetch_add(1, std::memory_order_relaxed));
      FILE_CNT(GlobalIdleWaitCalls.fetch_add(1, std::memory_order_relaxed));
      
      CV.wait(lock, [&] { return StopRequested || Reschedule; });
      
      FILE_CNT(GlobalWaitWakeups.fetch_add(1, std::memory_order_relaxed));
      if (Reschedule)
        FILE_CNT(GlobalIdleWaitReschedules.fetch_add(1, std::memory_order_relaxed));
      Reschedule = false;
      continue;
    }

    uint64_t earliest = nextToRunRaw->GetFlushTime();
    if (earliest == FileBackend::RIGHT_NOW)
    {
      FILE_CNT(GlobalHeadRightNowChecks.fetch_add(1, std::memory_order_relaxed));
      FILE_CNT(GlobalHeadRightNowSelected.fetch_add(1, std::memory_order_relaxed));
    }
    else
    {
      FILE_CNT(GlobalHeadScheduledChecks.fetch_add(1, std::memory_order_relaxed));
      uint64_t now = GetTimeInMillisec64();
      
      if (now < earliest)
      {
        uint64_t remaining = earliest - now;

        if (remaining > SCHEDULED_RUN_GAP_MS)
        {
          CurrentEarliestTime = earliest;
          FILE_CNT(GlobalWaitCalls.fetch_add(1, std::memory_order_relaxed));
          FILE_CNT(GlobalTimedWaitCalls.fetch_add(1, std::memory_order_relaxed));
          CV.wait_for(
            lock
            , std::chrono::milliseconds(remaining), [&] {
              return StopRequested || Reschedule;
            }
          );
          FILE_CNT(GlobalWaitWakeups.fetch_add(1, std::memory_order_relaxed));
          if (Reschedule)
            FILE_CNT(GlobalTimedWaitReschedules.fetch_add(1, std::memory_order_relaxed));
          else if (StopRequested == false)
            FILE_CNT(GlobalTimedWaitTimeouts.fetch_add(1, std::memory_order_relaxed));

          Reschedule = false;
          CurrentEarliestTime = UINT64_MAX;
          continue;
        }

        nextToRunRaw->FlushTime.store(
          FileBackend::RIGHT_NOW
          , std::memory_order_relaxed
        );
        FILE_CNT(GlobalScheduledEarlyRuns.fetch_add(1, std::memory_order_relaxed));
      }

      FILE_CNT(GlobalHeadScheduledSelected.fetch_add(1, std::memory_order_relaxed));
      FILE_CNT(GlobalDueScheduledRuns.fetch_add(1, std::memory_order_relaxed));
#if FILE_ENABLE_COUNTERS
      if (now > earliest)
      {
        uint64_t delay = now - earliest;
        GlobalPastDueScheduledRuns.fetch_add(1, std::memory_order_relaxed);
        GlobalScheduledLateRuns.fetch_add(1, std::memory_order_relaxed);
        GlobalTotalScheduledDelayMs.fetch_add(
          delay
          , std::memory_order_relaxed
        );
        UpdateMaxCounter(GlobalMaxScheduledDelayMs, delay);
      }
#endif
    }

    auto backendPos = std::find_if(
      Backends.begin()
      , Backends.end()
      , [nextToRunRaw](const FileBackendPtr& backend)
      {
        return backend.get() == nextToRunRaw;
      }
    );

    if (backendPos == Backends.end())
    {
      DeactivateBackend(nextToRunRaw);
      continue;
    }

    nextToRun = *backendPos;
    DeactivateBackend(nextToRunRaw);

    lock.unlock();
    
    FILE_CNT(GlobalWorkerRuns.fetch_add(1, std::memory_order_relaxed));
    if (!nextToRun->WorkerFunc())
    {
      lock.lock();
      RemoveBackendLocked(nextToRun);
      if (Backends.empty())
        StopRequested = true;
      lock.unlock();

      nextToRun->OnShutdown();

      lock.lock();
    }
    else
    {
      lock.lock();
      uint64_t flushTime = nextToRun->GetFlushTime();
      if (flushTime == 0)
        DeactivateBackend(nextToRun.get());
      else
        ActivateBackend(nextToRun.get(), flushTime);
    }
  }
}
