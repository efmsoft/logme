#pragma once 

#include <string>

namespace Logme
{
  class FileBackend;
  class FileManager;

  typedef std::shared_ptr<FileBackend> FileBackendPtr;

  class FileManagerFactory
  {
    std::recursive_mutex Lock;
    std::shared_ptr<FileManager> Instance;

  public:
    FileManagerFactory();
    ~FileManagerFactory();

    void Add(const FileBackendPtr& backend);
    void Notify(FileBackend* backend, uint64_t when);

    bool TestFileInUse(const std::string& file);
    void SetStopping();
  };
}