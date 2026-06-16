#include <atomic>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>

#include <Logme/File/CompressionManager.h>
#include <Logme/Logme.h>
#include <Logme/Utils.h>

#ifdef USE_ZLIB
#include <zlib.h>
#endif

using namespace Logme;

#ifdef USE_ZLIB
namespace
{
  std::atomic<std::uint64_t> TempFileCounter(0);

  bool EndsWith(
    const std::string& value
    , const std::string& suffix
  )
  {
    if (value.size() < suffix.size())
      return false;

    return value.compare(
      value.size() - suffix.size()
      , suffix.size()
      , suffix
    ) == 0;
  }

  std::string MakeTemporaryGzipName(const std::string& finalName)
  {
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    auto counter = TempFileCounter.fetch_add(1, std::memory_order_relaxed);

    return finalName
      + ".tmp."
      + std::to_string(now)
      + "."
      + std::to_string(counter);
  }
}
#endif

CompressionRegistration::CompressionRegistration()
  : Factory(nullptr)
  , Active(false)
{
}

CompressionRegistration::CompressionRegistration(CompressionManagerFactory* factory)
  : Factory(factory)
  , Active(factory != nullptr)
{
}

CompressionRegistration::~CompressionRegistration()
{
  Reset();
}

CompressionRegistration::CompressionRegistration(CompressionRegistration&& other) noexcept
  : Factory(other.Factory)
  , Active(other.Active)
{
  other.Factory = nullptr;
  other.Active = false;
}

CompressionRegistration& CompressionRegistration::operator=(
  CompressionRegistration&& other
) noexcept
{
  if (this == &other)
    return *this;

  Reset();

  Factory = other.Factory;
  Active = other.Active;
  other.Factory = nullptr;
  other.Active = false;

  return *this;
}

void CompressionRegistration::Submit(const std::string& file)
{
  if (!Active || Factory == nullptr)
    return;

  Factory->Submit(file);
}

void CompressionRegistration::Reset()
{
  if (!Active || Factory == nullptr)
    return;

  CompressionManagerFactory* factory = Factory;
  Factory = nullptr;
  Active = false;

  factory->ReleaseUser();
}

bool CompressionRegistration::IsActive() const
{
  return Active;
}

CompressionManager::CompressionManager(TFileInUseCallback testFileInUse)
  : StopRequested(false)
  , Stopped(true)
  , UserCount(0)
  , TestFileInUse(std::move(testFileInUse))
{
}

CompressionManager::~CompressionManager()
{
  SetStopping();
  Join();
}

void CompressionManager::Submit(const std::string& file)
{
  if (file.empty())
    return;

  std::lock_guard guard(Lock);

  if (StopRequested)
    return;

  Queue.push_back(file);
  StartWorkerLocked();
  CV.notify_all();
}

void CompressionManager::SetUserCount(std::size_t userCount)
{
  std::lock_guard guard(Lock);
  UserCount = userCount;

  if (UserCount == 0)
    CV.notify_all();
}

void CompressionManager::StopWhenQueueEmpty()
{
  std::lock_guard guard(Lock);
  UserCount = 0;
  CV.notify_all();
}

void CompressionManager::SetStopping()
{
  std::lock_guard guard(Lock);
  StopRequested = true;
  UserCount = 0;
  CV.notify_all();
}

void CompressionManager::Join()
{
  if (Worker.joinable())
    Worker.join();
}

bool CompressionManager::IsStopped()
{
  std::lock_guard guard(Lock);
  return Stopped;
}

void CompressionManager::StartWorkerLocked()
{
  if (Worker.joinable() && Stopped)
    Worker.join();

  if (Worker.joinable())
    return;

  StopRequested = false;
  Stopped = false;
  Worker = std::thread(&CompressionManager::WorkerFunc, this);
}

void CompressionManager::WorkerFunc()
{
  RenameThread(uint64_t(-1), "CompressionManager::WorkerFunc");

  for (;;)
  {
    std::string file;

    {
      std::unique_lock guard(Lock);
      CV.wait(
        guard
        , [this]()
        {
          return StopRequested || !Queue.empty() || UserCount == 0;
        }
      );

      if (Queue.empty())
      {
        if (StopRequested || UserCount == 0)
          break;

        continue;
      }

      file = std::move(Queue.front());
      Queue.erase(Queue.begin());
    }

    CompressFile(file);
  }

  std::lock_guard guard(Lock);
  Stopped = true;
}

void CompressionManager::CompressFile(const std::string& file)
{
#ifndef USE_ZLIB
  (void)file;
#else
  if (EndsWith(file, ".gz"))
    return;

  if (TestFileInUse && TestFileInUse(file))
  {
    LogmeW(CHINT, "compression skipped because file is in use: %s", file.c_str());
    return;
  }

  std::error_code ec;
  if (!std::filesystem::is_regular_file(file, ec) || ec)
    return;

  std::string finalName = file + ".gz";

  if (std::filesystem::exists(finalName, ec) && !ec)
  {
    LogmeW(CHINT, "compression skipped because target already exists: %s", finalName.c_str());
    return;
  }

  std::string tmpName = MakeTemporaryGzipName(finalName);

  std::ifstream input(file, std::ios::binary);
  if (!input)
  {
    LogmeE(CHINT, "failed to open compression source: %s", file.c_str());
    return;
  }

  gzFile output = gzopen(tmpName.c_str(), "wb");
  if (output == nullptr)
  {
    LogmeE(CHINT, "failed to open compression target: %s", tmpName.c_str());
    return;
  }

  bool ok = true;
  std::array<char, 64 * 1024> buffer;

  while (input)
  {
    input.read(buffer.data(), buffer.size());
    std::streamsize read = input.gcount();

    if (read <= 0)
      break;

    if (read > static_cast<std::streamsize>(std::numeric_limits<unsigned int>::max()))
    {
      ok = false;
      break;
    }

    int written = gzwrite(
      output
      , buffer.data()
      , static_cast<unsigned int>(read)
    );

    if (written != read)
    {
      ok = false;
      break;
    }
  }

  if (input.bad())
    ok = false;

  input.close();

  int closeResult = gzclose(output);
  if (closeResult != Z_OK)
    ok = false;

  if (!ok)
  {
    std::filesystem::remove(tmpName, ec);
    LogmeE(CHINT, "failed to compress file: %s", file.c_str());
    return;
  }

  std::filesystem::rename(tmpName, finalName, ec);
  if (ec)
  {
    std::filesystem::remove(tmpName, ec);
    LogmeE(CHINT, "failed to rename compressed file: %s", finalName.c_str());
    return;
  }

  std::filesystem::remove(file, ec);
  if (ec)
  {
    LogmeE(CHINT, "failed to delete compressed source: %s", file.c_str());
  }
#endif
}

CompressionManagerFactory::CompressionManagerFactory()
  : UserCount(0)
  , Stopping(false)
{
}

CompressionManagerFactory::CompressionManagerFactory(TFileInUseCallback testFileInUse)
  : TestFileInUse(std::move(testFileInUse))
  , UserCount(0)
  , Stopping(false)
{
}

CompressionManagerFactory::~CompressionManagerFactory()
{
  SetStopping();
}

CompressionRegistrationPtr CompressionManagerFactory::RegisterUser()
{
  std::lock_guard guard(Lock);

  if (Stopping)
    return std::make_unique<CompressionRegistration>();

  ++UserCount;
  if (Instance)
    Instance->SetUserCount(UserCount);

  return CompressionRegistrationPtr(new CompressionRegistration(this));
}

void CompressionManagerFactory::ReleaseUser()
{
  std::shared_ptr<CompressionManager> instance;
  std::size_t userCount = 0;

  {
    std::lock_guard guard(Lock);

    if (UserCount == 0)
      return;

    --UserCount;
    userCount = UserCount;
    instance = Instance;
  }

  if (!instance)
    return;

  instance->SetUserCount(userCount);

  if (userCount == 0)
    instance->StopWhenQueueEmpty();
}

void CompressionManagerFactory::Submit(const std::string& file)
{
#ifndef USE_ZLIB
  (void)file;
#else
  std::shared_ptr<CompressionManager> instance;

  {
    std::lock_guard guard(Lock);

    if (Stopping || UserCount == 0 || file.empty())
      return;

    if (Instance && Instance->IsStopped())
      Instance->Join();

    if (Instance == nullptr)
      Instance = std::make_shared<CompressionManager>(TestFileInUse);

    Instance->SetUserCount(UserCount);
    instance = Instance;
  }

  instance->Submit(file);
#endif
}

void CompressionManagerFactory::SetStopping()
{
  std::shared_ptr<CompressionManager> instance;

  {
    std::lock_guard guard(Lock);
    Stopping = true;
    UserCount = 0;
    instance = Instance;
  }

  if (instance)
  {
    instance->SetStopping();
    instance->Join();
  }
}
