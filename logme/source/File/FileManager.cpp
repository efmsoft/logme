#include <cassert>

#include <Logme/Backend/FileBackend.h>
#include <Logme/File/FileManager.h>
#include <Logme/Time/datetime.h> 
#include <Logme/Utils.h> 

using namespace Logme;

FileManager::FileManager(std::recursive_mutex& listLock)
  : ListLock(listLock)
  , Wake(false)
  , ShutdownFlag(false)
{
  Worker = std::thread(&FileManager::ManagementThread, this);
  assert(Worker.joinable());
}

FileManager::~FileManager()
{
  ShutdownFlag = true;

  if (Worker.joinable())
    Worker.join();
}

void FileManager::Add(FileBackend* backend)
{
  // This method is called with acquired ListLock!!!
  Backend.push_back(backend);
}

void FileManager::WakeUp()
{
  // This method is called with acquired ListLock!!!
  Wake = true;
}

bool FileManager::Empty() const
{
  // This method is called with acquired ListLock!!!
  return Backend.empty();
}

bool FileManager::TestFileInUse(const std::string& file)
{
  for (auto& b : Backend)
  {
    if (b->TestFileInUse(file))
      return true;
  }

  return false;
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
  RenameThread(-1, "FileManager::ManagementThread");

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
