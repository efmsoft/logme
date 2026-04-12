#include <Logme/Backend/FileBackend.h>
#include <Logme/File/FileManager.h>
#include <Logme/Time/datetime.h> 
#include <Logme/Utils.h> 
#include <cstdio>

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
  return out;
}

FileManager::FileManager()
  : StopRequested(false)
  , Reschedule(false)
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
    FILE_CNT(GlobalBackendShutdowns.fetch_add(1, std::memory_order_relaxed));
    backend->OnShutdown();
  }

#if FILE_ENABLE_COUNTERS
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
      "BackendRemovals=%llu\n"
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
      "FlushWaitCalls=%llu "
      "WorkerRuns=%llu "
      "WriteReadyCalls=%llu "
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
      , (unsigned long long)bc.FlushWaitCalls
      , (unsigned long long)bc.WorkerRuns
      , (unsigned long long)bc.WriteReadyCalls
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
  }
#endif
}

void FileManager::AddBackend(const FileBackendPtr& backend)
{
  FILE_CNT(GlobalAddBackendCalls.fetch_add(1, std::memory_order_relaxed));
  std::lock_guard lock(Lock);
  Backends.insert(backend);
  
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
  if (true)
  {
    std::lock_guard<std::mutex> lock(Lock);
    
    StopRequested = true;
    Reschedule = true;
  }

  CV.notify_all();

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
      backend->OnShutdown();

    Backends.clear();
  }
#endif
}

void FileManager::Notify(FileBackend* backend, uint64_t when)
{
  FILE_CNT(GlobalNotifyCalls.fetch_add(1, std::memory_order_relaxed));
  (void)backend;
  std::lock_guard lock(Lock);
  
  if (when == FileBackend::RIGHT_NOW || when < CurrentEarliestTime)
  {
    Reschedule = true;
    FILE_CNT(GlobalRescheduleSignals.fetch_add(1, std::memory_order_relaxed));
    CV.notify_all();
  }
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
    uint64_t earliest = UINT64_MAX;

    for (const auto& backend : Backends)
    {
      if (backend->HasEvents() == false)
        continue;

      if (backend->GetFlushTime() != FileBackend::RIGHT_NOW)
        continue;

      nextToRun = backend;
      earliest = 0;
      break;
    }

    if (nextToRun == nullptr)
    {
      for (const auto& backend : Backends)
      {
        if (backend->HasEvents() == false)
          continue;

        uint64_t flushTime = backend->GetFlushTime();
        if (flushTime > 0 && flushTime < earliest)
        {
          earliest = flushTime;
          nextToRun = backend;
        }
      }
    }

    if (nextToRun == nullptr)
    {
      CurrentEarliestTime = UINT64_MAX;
      FILE_CNT(GlobalWaitCalls.fetch_add(1, std::memory_order_relaxed));
      CV.wait(lock, [&] { return StopRequested || Reschedule; });
      FILE_CNT(GlobalWaitWakeups.fetch_add(1, std::memory_order_relaxed));
      Reschedule = false;
      continue;
    }

    if (earliest != 0)
    {
      uint64_t now = GetTimeInMillisec();
      
      // To avoid frequent sleeping and rapid wake-ups, it's better to
      // handle the request slightly earlier than the requested time.
      const uint64_t MIN_WAIT = 10;
      if (now + MIN_WAIT < earliest)
      {
        CurrentEarliestTime = earliest;
        FILE_CNT(GlobalWaitCalls.fetch_add(1, std::memory_order_relaxed));
        CV.wait_for(
          lock
          , std::chrono::milliseconds(earliest - now), [&] { 
            return StopRequested || Reschedule; 
          }
        );
        FILE_CNT(GlobalWaitWakeups.fetch_add(1, std::memory_order_relaxed));

        Reschedule = false;
        CurrentEarliestTime = UINT64_MAX;
        continue;
      }
    }

    lock.unlock();
    
    FILE_CNT(GlobalWorkerRuns.fetch_add(1, std::memory_order_relaxed));
    if (!nextToRun->WorkerFunc())
    {
      nextToRun->OnShutdown();
      
      lock.lock();
      Backends.erase(nextToRun);
      FILE_CNT(GlobalBackendRemovals.fetch_add(1, std::memory_order_relaxed));
      
      if (Backends.empty())
        StopRequested = true;
    }
    else 
      lock.lock();
  }
}
