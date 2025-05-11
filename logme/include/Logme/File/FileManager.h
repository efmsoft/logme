#pragma once

#include <mutex>
#include <thread>

namespace Logme
{
  class FileManager
  {
    std::recursive_mutex& ListLock;
    volatile bool Wake;

    typedef std::vector<class FileBackend*> BackendList;
    BackendList Backend;

    std::thread Worker;
    volatile bool ShutdownFlag;

  public:
    FileManager(std::recursive_mutex& listLock);
    ~FileManager();

    void Add(FileBackend* backend);
    void WakeUp();
    bool Empty() const;

    bool TestFileInUse(const std::string& file);

  private:
    void ManagementThread();
    bool DispatchEvents(size_t index, bool force);
  };
}
