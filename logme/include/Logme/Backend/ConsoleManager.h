#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <set>
#include <thread>

#include <Logme/Backend/ConsoleBackend.h>

namespace Logme
{
  class ConsoleManager
  {
    bool StopRequested;
    bool Reschedule;
    std::thread ManagerThread;

    std::mutex Lock;
    std::condition_variable CV;
    std::set<ConsoleBackendPtr> Backends;

#ifdef _WIN32
    uint64_t ThreadID;
#endif

  public:
    ConsoleManager();
    ~ConsoleManager();

    void AddBackend(const ConsoleBackendPtr& backend);
    void Notify(ConsoleBackend* backend);

    bool Stopping() const;
    void SetStopping();

  private:
    void ManagementThread();
  };
}
