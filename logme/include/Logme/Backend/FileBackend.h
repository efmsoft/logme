#pragma once

#include <Logme/Backend/Backend.h>
#include <Logme/DayChangeDetector.h>
#include <Logme/Buffer/BufferQueue.h>
#include <Logme/File/file_io.h>
#include <Logme/Obfuscate.h>
#include <Logme/Types.h>

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>

namespace Logme
{
  struct FileBackendConfig : public BackendConfig
  {
    bool Append;
    size_t MaxSize;
    std::string Filename;
    
    bool DailyRotation;
    int MaxParts;

    LOGMELNK FileBackendConfig();
    LOGMELNK ~FileBackendConfig();

    LOGMELNK bool Parse(const Json::Value* po) override;
  };

  struct FileBackendCounters
  {
    std::uint64_t DisplayCalls = 0;
    std::uint64_t AppendCalls = 0;
    std::uint64_t InputBytes = 0;
    std::uint64_t OutputBytes = 0;
    std::uint64_t FlushRequests = 0;
    std::uint64_t ImmediateFlushRequests = 0;
    std::uint64_t ScheduledFlushRequests = 0;
    std::uint64_t FlushAccepted = 0;
    std::uint64_t ImmediateFlushAccepted = 0;
    std::uint64_t ScheduledFlushAccepted = 0;
    std::uint64_t FlushRejected = 0;
    std::uint64_t ImmediateFlushRejected = 0;
    std::uint64_t ScheduledFlushRejected = 0;
    std::uint64_t FlushWaitCalls = 0;
    std::uint64_t WorkerRuns = 0;
    std::uint64_t WorkerImmediateRuns = 0;
    std::uint64_t WorkerTimedRuns = 0;
    std::uint64_t WorkerShutdownRuns = 0;
    std::uint64_t WriteReadyCalls = 0;
    std::uint64_t WriteBatches = 0;
    std::uint64_t WriteBatchBytes = 0;
    std::uint64_t WriteBatchMaxBytes = 0;
    std::uint64_t WrittenBuffers = 0;
    std::uint64_t WrittenBytes = 0;
    std::uint64_t WriteErrors = 0;
    std::uint64_t CreateLogCalls = 0;
    std::uint64_t CreateLogFailures = 0;
    std::uint64_t ChangePartCalls = 0;
    std::uint64_t ChangePartFailures = 0;
    std::uint64_t ShutdownCalls = 0;
    BufferCounters Queue;
  };

  class FileBackend 
    : public Backend
    , public FileIo
  {
    typedef std::vector<char> CharBuffer;
    typedef std::vector<size_t> SizeArray;

  private:
    bool Append;
    size_t MaxSize;
    size_t CurrentSize;
    size_t QueueSizeLimit;
    std::string Name;
    std::string NameTemplate;

    std::atomic<bool> Registered;
    std::atomic<bool> ShutdownFlag;
    std::atomic<bool> ShutdownCalled;
    std::atomic<uint64_t> FlushTime;

    std::mutex BufferLock;

    BufferQueue Queue;
    std::atomic<size_t> QueuedBytes;

    std::condition_variable Shutdown;
    std::condition_variable Done;

    DayChangeDetector Day;
    bool DailyRotation;
    int MaxParts;

    static size_t MaxSizeDefault;
    static size_t QueueSizeLimitDefault;

    NonceGen Nonce;
  
  public:
    enum 
    { 
      MAX_SIZE_DEFAULT = 8 * 1024 * 1024,
      
      QUEUE_BUFFER_SIZE = 256 * 1024,       // buffer size
      MAX_TOTAL_BUFFERS = 128,              // hard limit for queue buffers
      FLUSH_PRESSURE_BUFFERS = 96,          // force processing if used buffers >= limit
      QUEUE_SIZE_LIMIT = QUEUE_BUFFER_SIZE * FLUSH_PRESSURE_BUFFERS,
      STAT_OUTPUT_PERIOD = 10 * 60 * 1000,  // 10 min

      RIGHT_NOW = 1,                        // Force flush right now
      FLUSH_AFTER = 100,
    };

    constexpr static const char* TYPE_ID = "FileBackend";

    LOGMELNK FileBackend(ChannelPtr owner);
    LOGMELNK ~FileBackend();

    LOGMELNK void SetAppend(bool append);
    LOGMELNK void SetMaxSize(size_t size);
    LOGMELNK void SetQueueLimit(size_t size);

    LOGMELNK bool CreateLog(const char* name);
    LOGMELNK void CloseLog();

    LOGMELNK std::string GetPathName(int index = 0) override;

    LOGMELNK BackendConfigPtr CreateConfig() override;
    LOGMELNK bool ApplyConfig(BackendConfigPtr c) override;
    LOGMELNK bool IsIdle() const override;
    LOGMELNK void Freeze() override;
    LOGMELNK void Flush() override;
    LOGMELNK std::string FormatDetails() override;

    LOGMELNK static size_t GetMaxSizeDefault();
    LOGMELNK static void SetMaxSizeDefault(size_t size);

    LOGMELNK static size_t GetQueueSizeLimitDefault();
    LOGMELNK static void SetQueueSizeLimitDefault(size_t size);

    LOGMELNK bool TestFileInUse(const std::string& file) const;
    LOGMELNK size_t GetSize();

    LOGMELNK uint64_t GetFlushTime() const;
    LOGMELNK bool HasEvents() const;
    LOGMELNK void AppendString(const char* text, size_t len);
    LOGMELNK static FileBackendCounters GetCounters();
  
  protected:
    LOGMELNK void Display(Context& context) override;
    void AppendStringInternal(const char* text, size_t len);

  private:
    class FileManagerFactory& GetFactory() const;

    void Truncate();
    void AppendObfuscated(const char* text, size_t add);
    void AppendOutputData(const char* text, size_t add);
    void RequestFlush(uint64_t when = RIGHT_NOW);
    bool WriteReadyData();
    void UpdateFlushTimeAfterWork();

    void WaitForShutdown();


    friend class FileManager;
    bool WorkerFunc();
    void OnShutdown();

    bool ChangePart();
  };

  typedef std::shared_ptr<class FileBackend> FileBackendPtr;
}
