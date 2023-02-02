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
  ListLock.lock();

  if (Instance == nullptr)
    Instance = std::make_shared<FileManager>(ListLock);

  Instance->Add(backend);
  ListLock.unlock();
}

void FileManagerFactory::Remove(FileBackend* backend)
{
  std::shared_ptr<FileManager> instance;

  ListLock.lock();

  if (Instance && Instance->Empty())
    Instance.swap(instance);

  ListLock.unlock();
}

void FileManagerFactory::WakeUp()
{
  ListLock.lock();

  if (Instance)
    Instance->WakeUp();

  ListLock.unlock();
}
