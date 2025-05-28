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