#pragma once 

#include <memory>
#include <string>

#include <Logme/CritSection.h>
#include <Logme/Buffer/DataBuffer.h>

namespace Logme
{
  class FileBackend;
  class FileManager;
  class MemoryUsageTracker;

  typedef std::shared_ptr<FileBackend> FileBackendPtr;

  class FileManagerFactory
  {
    CS Lock;
    std::shared_ptr<FileManager> Instance;

  public:
    FileManagerFactory();
    ~FileManagerFactory();

    void Add(const FileBackendPtr& backend);
    void Notify(FileBackend* backend, uint64_t when);

    bool TestFileInUse(const std::string& file);
    void SetStopping();

    DataBufferPtr TakeDataBuffer(
      MemoryUsageTracker* memoryTracker
      , std::size_t capacity
    );
    bool ReturnDataBuffer(DataBufferPtr buffer, std::size_t cacheLimit);
    void TrimDataBufferCache(std::size_t cacheLimit);
  };
}