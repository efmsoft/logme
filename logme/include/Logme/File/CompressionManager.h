#pragma once

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <Logme/CritSection.h>
#include <Logme/Types.h>

namespace Logme
{
  typedef std::function<bool(const std::string&)> TFileInUseCallback;

  class CompressionManager;
  class CompressionManagerFactory;

  class CompressionRegistration
  {
    CompressionManagerFactory* Factory;
    bool Active;

  public:
    LOGMELNK CompressionRegistration();
    LOGMELNK ~CompressionRegistration();

    CompressionRegistration(const CompressionRegistration&) = delete;
    CompressionRegistration& operator=(const CompressionRegistration&) = delete;

    LOGMELNK CompressionRegistration(CompressionRegistration&& other) noexcept;
    LOGMELNK CompressionRegistration& operator=(CompressionRegistration&& other) noexcept;

    LOGMELNK void Submit(const std::string& file);
    LOGMELNK void Reset();
    LOGMELNK bool IsActive() const;

  private:
    friend class CompressionManagerFactory;

    explicit CompressionRegistration(CompressionManagerFactory* factory);
  };

  typedef std::unique_ptr<CompressionRegistration> CompressionRegistrationPtr;

  class CompressionManager
  {
    bool StopRequested;
    bool Stopped;
    std::size_t UserCount;
    std::thread Worker;

    std::mutex Lock;
    std::condition_variable CV;
    std::vector<std::string> Queue;
    TFileInUseCallback TestFileInUse;

  public:
    LOGMELNK explicit CompressionManager(TFileInUseCallback testFileInUse);
    LOGMELNK ~CompressionManager();

    LOGMELNK void Submit(const std::string& file);
    LOGMELNK void SetUserCount(std::size_t userCount);
    LOGMELNK void StopWhenQueueEmpty();
    LOGMELNK void SetStopping();
    LOGMELNK void Join();
    LOGMELNK bool IsStopped();

  private:
    void StartWorkerLocked();
    void WorkerFunc();
    void CompressFile(const std::string& file);
  };

  class CompressionManagerFactory
  {
    CS Lock;
    std::shared_ptr<CompressionManager> Instance;
    TFileInUseCallback TestFileInUse;
    std::size_t UserCount;
    bool Stopping;

  public:
    LOGMELNK CompressionManagerFactory();
    LOGMELNK explicit CompressionManagerFactory(TFileInUseCallback testFileInUse);
    LOGMELNK ~CompressionManagerFactory();

    LOGMELNK CompressionRegistrationPtr RegisterUser();
    LOGMELNK void ReleaseUser();
    LOGMELNK void Submit(const std::string& file);
    LOGMELNK void SetStopping();
  };
}
