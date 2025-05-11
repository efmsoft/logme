#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace Logme
{
  typedef std::function<bool(const std::string&)> TTestFileInUse;

  class DirectorySizeWatchdog
  {
    const std::string& TargetDirectory;
    TTestFileInUse TestInUse;

    std::vector<std::string> Extensions;
    uint64_t MaximalSize;
    int CheckPeriodicity;

    std::mutex Lock;
    std::atomic<bool> StopFlag;
    std::condition_variable StopCondition;
    std::thread Worker;

  public:
    DirectorySizeWatchdog(const std::string& target, TTestFileInUse testInUse);
    ~DirectorySizeWatchdog();

    bool Run();
    void Stop();

    void SetMaximalSize(uint64_t size);
    uint64_t GetMaximalSize() const;

    void SetPeriodicity(int Periodicity);
    int GetPeriodicity(int Periodicity) const;

    void SetExtensions(const std::vector<std::string>& extensions);
    const std::vector<std::string> GetExtensions() const;

    void SetTestInUse(TTestFileInUse testInUse);

  private:
    void WorkerProc();
 
    struct FileInfo 
    {
      uintmax_t Size;
      std::filesystem::path Path;
      std::chrono::system_clock::time_point LastWrite;
    };

    void Monitor();
    bool LimitExeeded(uintmax_t& total_size);
    
    std::deque<FileInfo> Collect(uintmax_t total_size);
    void DeleteFiles(const std::deque<FileInfo>& files);

    void InsertSorted(
      std::deque<FileInfo>& files
      , const FileInfo& new_file
      , uintmax_t max_batch_size
    );
  };
}