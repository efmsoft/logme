#include <Logme/Backend/FileBackend.h>
#include <Logme/File/FileManager.h>
#include <Logme/Time/datetime.h> 
#include <Logme/Utils.h> 

using namespace Logme;

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
    backend->OnShutdown();
}

void FileManager::AddBackend(const FileBackendPtr& backend)
{
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
  std::lock_guard lock(Lock);
  
  if (when == FileBackend::RIGHT_NOW || when < CurrentEarliestTime)
  {
    Reschedule = true;
    CV.notify_all();
  }
}

bool FileManager::TestFileInUse(const std::string& file)
{
  std::lock_guard lock(Lock);

  for (const auto& backend : Backends)
    if (backend->TestFileInUse(file))
      return true;
  
  return false;
}

void FileManager::ManagementThread()
{
  RenameThread(-1, "FileManager::ManagementThread");

#ifdef _WIN32
  ThreadID = GetCurrentThreadId();
#endif

  std::unique_lock lock(Lock);

  while (StopRequested == false)
  {
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
      CV.wait(lock, [&] { return StopRequested || Reschedule; });
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
        CV.wait_for(
          lock
          , std::chrono::milliseconds(earliest - now), [&] { 
            return StopRequested || Reschedule; 
          }
        );

        Reschedule = false;
        CurrentEarliestTime = UINT64_MAX;
        continue;
      }
    }

    lock.unlock();
    
    if (!nextToRun->WorkerFunc())
    {
      nextToRun->OnShutdown();
      
      lock.lock();
      Backends.erase(nextToRun);
      
      if (Backends.empty())
        StopRequested = true;
    }
    else 
      lock.lock();
  }
}
