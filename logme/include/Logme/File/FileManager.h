#pragma once

#include <condition_variable>
#include <mutex>
#include <set>
#include <stdint.h>
#include <thread>

namespace Logme
{
  class FileManager
  {
    bool StopRequested;
    bool Reschedule;
    std::thread ManagerThread;

    std::mutex Lock;
    std::condition_variable CV;
    std::set<FileBackendPtr> Backends;

    uint64_t CurrentEarliestTime;

  public:
    FileManager();
    ~FileManager();

    void AddBackend(const FileBackendPtr& backend);
    void Notify(FileBackend* backend, uint64_t when);

    bool Stopping() const;
    void SetStopping();

    bool TestFileInUse(const std::string& file);

  private:
    void ManagementThread();
  };
}
