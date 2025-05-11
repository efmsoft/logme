#include <Logme/Backend/FileBackend.h>
#include <Logme/File/FileManager.h>
#include <Logme/File/FileManagerFactory.h>

using namespace Logme;

FileManagerFactory::FileManagerFactory()
{
}

FileManagerFactory::~FileManagerFactory()
{
}

void FileManagerFactory::Add(FileBackend* backend)
{
  std::lock_guard guard(ListLock);

  if (Instance == nullptr)
    Instance = std::make_shared<FileManager>(ListLock);

  Instance->Add(backend);
}

void FileManagerFactory::Remove(FileBackend* backend)
{
  std::shared_ptr<FileManager> instance;
  std::lock_guard guard(ListLock);

  if (Instance && Instance->Empty())
    Instance.swap(instance);
}

void FileManagerFactory::WakeUp()
{
  std::lock_guard guard(ListLock);

  if (Instance)
    Instance->WakeUp();
}

bool FileManagerFactory::TestFileInUse(const std::string& file)
{
  std::lock_guard guard(ListLock);

  if (Instance)
    return Instance->TestFileInUse(file);

  return false;
}