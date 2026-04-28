#include <Logme/Backend/ConsoleManager.h>
#include <Logme/Utils.h>

using namespace Logme;

ConsoleManager::ConsoleManager()
  : StopRequested(false)
  , Reschedule(false)
#ifdef _WIN32
  , ThreadID(0)
#endif
{
}

ConsoleManager::~ConsoleManager()
{
  SetStopping();

  if (ManagerThread.joinable())
    ManagerThread.join();

  for (const auto& backend : Backends)
    backend->OnShutdown();
}

void ConsoleManager::AddBackend(const ConsoleBackendPtr& backend)
{
  std::lock_guard lock(Lock);
  Backends.insert(backend);

  if (!ManagerThread.joinable())
    ManagerThread = std::thread(&ConsoleManager::ManagementThread, this);
}

bool ConsoleManager::Stopping() const
{
  return StopRequested;
}

#ifdef _WIN32
#include <windows.h>

static bool ThreadExists(uint64_t threadIdValue)
{
  DWORD threadId = static_cast<DWORD>(threadIdValue);
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

void ConsoleManager::SetStopping()
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

void ConsoleManager::ManagementThread()
{
  RenameThread(uint64_t(-1), "ConsoleManager::ManagementThread");

#ifdef _WIN32
  ThreadID = GetCurrentThreadId();
#endif

  for (;;)
  {
    std::set<ConsoleBackendPtr> backends;
    bool stopping = false;

    {
      std::unique_lock<std::mutex> lock(Lock);
      CV.wait(lock, [this]() { return StopRequested || Reschedule; });

      Reschedule = false;
      stopping = StopRequested;
      backends = Backends;
    }

    for (auto& backend : backends)
    {
      bool keep = backend->WorkerFunc();
      if (keep && !stopping)
        continue;

      std::lock_guard<std::mutex> lock(Lock);
      auto pos = Backends.find(backend);
      if (pos != Backends.end())
      {
        (*pos)->OnShutdown();
        Backends.erase(pos);
      }
    }

    if (stopping)
    {
      std::lock_guard<std::mutex> lock(Lock);
      if (Backends.empty())
        break;

      Reschedule = true;
    }
  }

#ifdef _WIN32
  ThreadID = 0;
#endif
}
