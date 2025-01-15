#pragma once 

namespace Logme
{
  class FileBackend;
  class FileManager;

  class FileManagerFactory
  {
    std::recursive_mutex ListLock;
    std::shared_ptr<FileManager> Instance;

  public:
    FileManagerFactory();
    ~FileManagerFactory();

    void Add(FileBackend* backend);
    void Remove(FileBackend* backend);
    void WakeUp();
  };
}