#include <Logme/Backend/FileBackend.h>
#include <Logme/Backend/FileManager.h>
#include <Logme/Time/datetime.h> 

using namespace Logme;

std::mutex FileManager::ListLock;
std::shared_ptr<FileManager> FileManager::Instance;
volatile bool FileManager::Wake;

FileManager::FileManager()
  : ShutdownFlag(false)
  , Worker(&FileManager::ManagementThread, this)
{
}

FileManager::~FileManager()
{
  ShutdownFlag = true;

  if (Worker.joinable())
    Worker.join();
}

void FileManager::Add(FileBackend* backend)
{
  ListLock.lock();

  if (Instance == nullptr)
    Instance = std::make_shared<FileManager>();

  Instance->Backend.push_back(backend);
  ListLock.unlock();
}

void FileManager::Remove(FileBackend* backend)
{
  std::shared_ptr<FileManager> instance;

  ListLock.lock();

  if (Instance && !Instance->Backend.size())
    Instance.swap(instance);

  ListLock.unlock();
}

void FileManager::WakeUp()
{
  Wake = true;
}

bool FileManager::DispatchEvents(size_t index, bool force)
{
  FileBackend* backend = Backend[index];
  if (backend->WorkerFunc(force) && !ShutdownFlag)
    return true;

  backend->OnShutdown();
  Backend.erase(Backend.begin() + index);
  return false;
}

void FileManager::ManagementThread()
{
  // Used to combine data from several consecutive Log() calls
  const unsigned delay = 100;
  for (unsigned t0 = GetTimeInMillisec();; Sleep(10))
  {
    if (ShutdownFlag && Backend.empty())
      break;

    unsigned t1 = GetTimeInMillisec();
    if (t1 < t0)
      t0 = 0;

    // Wake is true if one of backends requested flush.
    // Otherwise force flush once per 100ms
    if (!Wake && t1 - t0 < delay)
      continue;

    ListLock.lock();

    bool force = t1 - t0 >= delay;
    Wake = false;
    if (force)
      t0 = t1;

    for (size_t i = 0; i < Backend.size();)
      if (DispatchEvents(i, force))
        i++;

    ListLock.unlock();
  }
}
