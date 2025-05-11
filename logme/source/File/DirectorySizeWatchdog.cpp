#include <filesystem>
#include <unordered_set>

#include <Logme/File/DirectorySizeWatchdog.h>
#include <Logme/Logme.h>
#include <Logme/Utils.h>

using namespace Logme;

namespace fs = std::filesystem;

DirectorySizeWatchdog::DirectorySizeWatchdog(
  const std::string& target
  , TTestFileInUse testInUse
)
  : TargetDirectory(target)
  , TestInUse(testInUse)
  , MaximalSize(-1)
  , CheckPeriodicity(0)
  , StopFlag(false)
{
}

DirectorySizeWatchdog::~DirectorySizeWatchdog()
{
  Stop();
}

void DirectorySizeWatchdog::SetMaximalSize(uint64_t size)
{
  MaximalSize = size;
}

uint64_t DirectorySizeWatchdog::GetMaximalSize() const
{
  return MaximalSize;
}

void DirectorySizeWatchdog::SetPeriodicity(int periodicity)
{
  CheckPeriodicity = periodicity;
}

int DirectorySizeWatchdog::GetPeriodicity(int Periodicity) const
{
  return CheckPeriodicity;
}

void DirectorySizeWatchdog::SetExtensions(const std::vector<std::string>& extensions)
{
  Extensions = extensions;
}

const std::vector<std::string> DirectorySizeWatchdog::GetExtensions() const
{
  return Extensions;
}

void DirectorySizeWatchdog::SetTestInUse(TTestFileInUse testInUse)
{
  TestInUse = testInUse;
}

void DirectorySizeWatchdog::Stop()
{
  if (Worker.joinable())
  {
    StopFlag.store(true);
    StopCondition.notify_one();

    Worker.join();
  }
}

bool DirectorySizeWatchdog::Run()
{
  if (Worker.joinable())
    return false;

  if (Extensions.empty())
    return false;

  if (MaximalSize == 0 || MaximalSize == -1)
    return false;

  if (CheckPeriodicity == 0 || CheckPeriodicity == -1)
    return false;

  StopFlag = false;
  Worker = std::thread(&DirectorySizeWatchdog::WorkerProc, this);

  return true;
}

void DirectorySizeWatchdog::WorkerProc()
{
  RenameThread(-1, "DirectorySizeWatchdog");

  while (!StopFlag.load()) 
  {
    Monitor();
 
    std::unique_lock<std::mutex> lock(Lock);
    StopCondition.wait_for(
      lock
      , std::chrono::milliseconds(CheckPeriodicity)
      , [this]{ return StopFlag.load(); }
    );
  }
}

bool DirectorySizeWatchdog::LimitExeeded(uintmax_t& total_size)
{
  if (Extensions.empty() || MaximalSize == 0 || MaximalSize == -1)
    return false;

  std::unordered_set<std::string> ext_set(Extensions.begin(), Extensions.end());

  auto rdi = fs::recursive_directory_iterator(
    TargetDirectory
    , fs::directory_options::skip_permission_denied
  );

  total_size = 0;

  for (const auto& e : rdi)
  {
    if (StopFlag.load()) 
      break;

    if (!fs::is_regular_file(e.status())) 
      continue;

    auto ext = e.path().extension().string();
    if (ext_set.find(ext) == ext_set.end()) continue;

    std::error_code ec;
    uintmax_t fsize = fs::file_size(e, ec);
    if (!ec)
      total_size += fsize;
  }

  if (total_size > MaximalSize)
  {
    fLogmeW(
      CHINT
      , "The log directory size limit has been exceeded {} > {}"
      , total_size
      , MaximalSize
    );

    return true;
  }

  return false;
}

void DirectorySizeWatchdog::InsertSorted(
  std::deque<FileInfo>& files
  , const FileInfo& new_file
  , uintmax_t max_batch_size
) 
{
  auto pos = std::upper_bound(
    files.begin()
    , files.end()
    , new_file
    , [](const FileInfo& a, const FileInfo& b){ 
      return a.LastWrite < b.LastWrite; 
    }
  );
  
  files.insert(pos, new_file);

  uintmax_t current_size = 0;
  for (auto it = files.rbegin(); it != files.rend(); ++it) 
  {
    current_size += it->Size;
  
    if (current_size > max_batch_size) 
    {
      files.erase(files.begin(), it.base());
      break;
    }
  }
}

void DirectorySizeWatchdog::DeleteFiles(
  const std::deque<FileInfo>& files
)
{
  LogmeI(CHINT, "Deleting %zu files", files.size());

  for (const auto& file : files) 
  {
    std::error_code ec;
    fs::remove(file.Path, ec);

    if (ec)
    {
      fLogmeE(CHINT, "unable to delete {}: {}", file.Path.string(), ec.message());
    }
    else
      fLogmeE(CHINT, "{} was deleted", file.Path.string());
  }
}

std::deque<DirectorySizeWatchdog::FileInfo> DirectorySizeWatchdog::Collect(
  uintmax_t total_size
)
{
  const size_t maxBatch = 32ULL * 1024 * 1024;
  uintmax_t max_batch_size = MaximalSize / 20;
  
  if (max_batch_size > maxBatch)
    max_batch_size = maxBatch;

  if (total_size > MaximalSize)
    max_batch_size += total_size - MaximalSize;

  std::unordered_set<std::string> ext_set(Extensions.begin(), Extensions.end());

  std::deque<FileInfo> candidate_files;

  auto rdi = fs::recursive_directory_iterator(
    TargetDirectory
    , fs::directory_options::skip_permission_denied
  );

  for (const auto& entry : rdi) 
  {
    if (StopFlag.load()) 
      break;

    if (!fs::is_regular_file(entry.status())) 
      continue;

    auto ext = entry.path().extension().string();
    if (ext_set.find(ext) == ext_set.end()) continue;

    if (TestInUse && TestInUse(entry.path().string()))
      continue;

    std::error_code ec;
    uintmax_t fsize = fs::file_size(entry, ec);
    if (ec)
      continue;

    auto ftime = fs::last_write_time(entry, ec);
    if (ec)
      continue;

    auto sctp = std::chrono::clock_cast<std::chrono::system_clock>(ftime);

    InsertSorted(
      candidate_files
      , { fsize, entry.path(), sctp }
      , max_batch_size
    );
  }

  return candidate_files;
}

void DirectorySizeWatchdog::Monitor()
{
  uintmax_t total_size = 0;
  if (!LimitExeeded(total_size))
    return;

  auto candidate = Collect(total_size);
  DeleteFiles(candidate);
}