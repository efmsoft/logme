#pragma once

#include <Logme/Buffer/BufferCounters.h>

#include <condition_variable>
#include <mutex>
#include <set>
#include <stdint.h>
#include <thread>

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
  };

  class FileManager
  {
    bool StopRequested;
    bool Reschedule;
    std::thread ManagerThread;

    std::mutex Lock;
    std::condition_variable CV;
    std::set<FileBackendPtr> Backends;

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
    void ManagementThread();
  };
}
