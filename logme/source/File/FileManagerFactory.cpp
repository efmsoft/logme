#include <Logme/Backend/FileBackend.h>
#include <Logme/File/FileManager.h>
#include <Logme/File/FileManagerFactory.h>

using namespace Logme;

FileManagerFactory::FileManagerFactory()
{
}

FileManagerFactory::~FileManagerFactory()
{
  Instance.reset();
}

void FileManagerFactory::Add(const FileBackendPtr& backend)
{
  std::unique_lock guard(Lock);

  if (Instance && Instance->Stopping())
  {
    std::shared_ptr<FileManager> instance;
    Instance.swap(instance);
  }

  if (Instance == nullptr)
    Instance = std::make_shared<FileManager>();

  Instance->AddBackend(backend);
}

void FileManagerFactory::Notify(FileBackend* backend, uint64_t when)
{
  std::unique_lock guard(Lock);
  std::shared_ptr<FileManager> instance = Instance;
  guard.unlock();

  if (instance)
    instance->Notify(backend, when);
}

bool FileManagerFactory::TestFileInUse(const std::string& file)
{
  std::unique_lock guard(Lock);
  std::shared_ptr<FileManager> instance = Instance;
  guard.unlock();

  if (instance)
    return instance->TestFileInUse(file);

  return false;
}

void FileManagerFactory::SetStopping()
{
  std::unique_lock guard(Lock);
  std::shared_ptr<FileManager> instance = Instance;
  guard.unlock();

  if (instance)
    instance->SetStopping();
}
DataBufferPtr FileManagerFactory::TakeDataBuffer(
  MemoryUsageTracker* memoryTracker
  , std::size_t capacity
  , std::size_t cacheLimit
  , std::size_t cacheMaxLimit
  , std::uint64_t retainOverLimitMs
)
{
  std::unique_lock guard(Lock);
  std::shared_ptr<FileManager> instance = Instance;
  guard.unlock();

  if (!instance)
    return nullptr;

  return instance->TakeDataBuffer(
    memoryTracker
    , capacity
    , cacheLimit
    , cacheMaxLimit
    , retainOverLimitMs
  );
}

bool FileManagerFactory::ReturnDataBuffer(
  DataBufferPtr buffer
  , std::size_t cacheLimit
  , std::size_t cacheMaxLimit
  , std::uint64_t retainOverLimitMs
)
{
  std::unique_lock guard(Lock);
  std::shared_ptr<FileManager> instance = Instance;
  guard.unlock();

  if (!instance)
    return false;

  return instance->ReturnDataBuffer(
    std::move(buffer)
    , cacheLimit
    , cacheMaxLimit
    , retainOverLimitMs
  );
}

void FileManagerFactory::TrimDataBufferCache(std::size_t cacheLimit)
{
  std::unique_lock guard(Lock);
  std::shared_ptr<FileManager> instance = Instance;
  guard.unlock();

  if (instance)
    instance->TrimDataBufferCache(cacheLimit);
}

void FileManagerFactory::TrimDataBufferCacheMaxLimit(std::size_t cacheMaxLimit)
{
  std::unique_lock guard(Lock);
  std::shared_ptr<FileManager> instance = Instance;
  guard.unlock();

  if (instance)
    instance->TrimDataBufferCacheMaxLimit(cacheMaxLimit);
}

void FileManagerFactory::ClearDataBufferCache()
{
  std::unique_lock guard(Lock);
  std::shared_ptr<FileManager> instance = Instance;
  guard.unlock();

  if (instance)
    instance->ClearDataBufferCache();
}
