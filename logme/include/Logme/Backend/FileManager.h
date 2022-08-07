#pragma once

#include <mutex>
#include <thread>

namespace Logme
{
  class FileBackend;

  class FileManager
  {
  public:
    static void Add(FileBackend* backend);
    static void Remove(FileBackend* backend);
    static void WakeUp();

    FileManager();
    ~FileManager();

    void ManagementThread();
    bool DispatchEvents(size_t index, bool force);

  private:
    static std::mutex ListLock;
    static std::shared_ptr<FileManager> Instance;
    static volatile bool Wake;

    typedef std::vector<FileBackend*> BackendList;
    BackendList Backend;

    std::thread Worker;
    volatile bool ShutdownFlag;
  };
}
