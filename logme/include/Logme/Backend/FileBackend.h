#pragma once

#include <atomic>
#include <condition_variable>
#include <ctime>
#include <memory>
#include <mutex>
#include <regex>
#include <vector>

#include <Logme/Backend/MemoryTrackedBackend.h>
#include <Logme/Buffer/BufferQueue.h>
#include <Logme/File/CompressionManager.h>
#include <Logme/File/buffered_file_io.h>
#include <Logme/File/file_io.h>
#include <Logme/Obfuscate.h>
#include <Logme/Types.h>

#ifndef FILE_ENABLE_FLUSH_SOURCE_COUNTERS
#define FILE_ENABLE_FLUSH_SOURCE_COUNTERS 0
#endif

namespace Logme
{
  enum SizeLimitPolicy
  {
    SIZE_LIMIT_TRUNCATE,
    SIZE_LIMIT_ROTATE,
  };

  enum TimeRotationMode
  {
    TIME_ROTATION_NONE,
    TIME_ROTATION_HOURLY,
    TIME_ROTATION_DAILY,
    TIME_ROTATION_WEEKLY,
    TIME_ROTATION_MONTHLY,
  };

  struct FileBackendConfig : public BackendConfig
  {
    bool Append;
    size_t MaxSize;
    SizeLimitPolicy OnSizeLimit;
    std::string Filename;
    std::string ArchiveFilename;
    
    bool DailyRotation;
    TimeRotationMode TimeRotation;
    int MaxParts;
    uint64_t RetentionMaxAge;
    uint64_t RetentionMaxTotalSize;
    bool RetentionCleanOnStart;
    bool GzipCompression;

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

#if FILE_ENABLE_WRITE_READY_COUNTERS
    std::uint64_t WriteReadyEmptyCalls = 0;
    std::uint64_t WriteReadyRawCalls = 0;
    std::uint64_t WriteReadyRawBytes = 0;
    std::uint64_t WriteReadyRawMaxBytes = 0;
    std::uint64_t WriteReadyMaxBuffers = 0;
    std::uint64_t WriteReadyMaxBytes = 0;
    std::uint64_t WorkerWriteLoopIterations = 0;
    std::uint64_t WorkerWriteLoopMaxIterations = 0;
    std::uint64_t WorkerPublishCurrentCalls = 0;
    std::uint64_t WorkerPublishCurrentSuccess = 0;
    std::uint64_t WorkerBreaks = 0;
#endif

#if FILE_ENABLE_FLUSH_SOURCE_COUNTERS
    std::uint64_t PublishFromFlushCalls = 0;
    std::uint64_t PublishFromFlushSuccess = 0;
    std::uint64_t PublishFromWorkerImmediateCalls = 0;
    std::uint64_t PublishFromWorkerImmediateSuccess = 0;
    std::uint64_t PublishFromWorkerShutdownCalls = 0;
    std::uint64_t PublishFromWorkerShutdownSuccess = 0;
    std::uint64_t PublishFromOnShutdownCalls = 0;
    std::uint64_t PublishFromOnShutdownSuccess = 0;
    std::uint64_t RequestRightNowFromFlush = 0;
    std::uint64_t RequestRightNowFromFreeze = 0;
    std::uint64_t RequestRightNowFromPressureBuffers = 0;
    std::uint64_t RequestRightNowFromPressureBytes = 0;
    std::uint64_t RequestRightNowFromPressureBoth = 0;
    std::uint64_t RequestScheduledFromFirstData = 0;
    std::uint64_t PressureByBuffers = 0;
    std::uint64_t PressureByBytes = 0;
    std::uint64_t PressureByBoth = 0;
    std::uint64_t PublishCurrentQueuedBytes = 0;
    std::uint64_t PublishCurrentMaxQueuedBytes = 0;
    std::uint64_t PublishCurrentAgeMs = 0;
    std::uint64_t PublishCurrentMaxAgeMs = 0;
#endif
    std::uint64_t CreateLogCalls = 0;
    std::uint64_t CreateLogFailures = 0;
    std::uint64_t ChangePartCalls = 0;
    std::uint64_t ChangePartFailures = 0;
    std::uint64_t SizeLimitCompletionCalls = 0;
    std::uint64_t TimeLimitCompletionCalls = 0;
    std::uint64_t ArchivedFiles = 0;
    std::uint64_t CompressionSubmitCalls = 0;
    std::uint64_t RetentionRuns = 0;
    std::uint64_t ShutdownCalls = 0;
    BufferCounters Queue;
  };

  // File sink with built-in file-size control, daily rotation, part switching,
  // asynchronous buffering, and cooperation with the log directory retention
  // watchdog. In comparisons with other logging libraries this covers the
  // usual rotating-file-sink and retention use cases.
  class FileArchivePolicy;
  class FileTimeRotationPolicy;

  class FileBackend 
    : public MemoryTrackedBackend
    , public FileIo
  {
    typedef std::vector<char> CharBuffer;
    typedef std::vector<size_t> SizeArray;

  private:
    enum FileCompletionReason
    {
      FILE_COMPLETION_TIME_LIMIT,
      FILE_COMPLETION_SIZE_LIMIT,
    };

    bool Append;
    size_t MaxSize;
    SizeLimitPolicy OnSizeLimit;
    size_t CurrentSize;
    size_t QueueSizeLimit;
    uint64_t FlushAfter;
    std::string Name;
    std::string NameTemplate;
    std::unique_ptr<FileArchivePolicy> ArchivePolicy;

    std::atomic<bool> Registered;
    std::atomic<bool> ShutdownFlag;
    std::atomic<bool> ShutdownCalled;
    std::atomic<uint64_t> FlushTime;

    FileBackend* ActiveNext;
    FileBackend* ActivePrev;
    bool ActiveLinked;

    std::mutex BufferLock;

    BufferQueue Queue;
    std::atomic<size_t> QueuedBytes;

    std::condition_variable Shutdown;
    std::condition_variable Done;

    std::unique_ptr<FileTimeRotationPolicy> TimeRotationPolicy;
    int MaxParts;
    uint64_t RetentionMaxAge;
    uint64_t RetentionMaxTotalSize;
    bool RetentionCleanOnStart;
    bool GzipCompression;
    CompressionRegistrationPtr Compression;

    static size_t MaxSizeDefault;
    static size_t QueueSizeLimitDefault;
    static size_t QueueBufferLimitDefault;
    static uint64_t FlushAfterDefault;
    static size_t DataBufferSizeDefault;
    static std::atomic<size_t> DataBufferCacheLimit;
    static std::atomic<size_t> DataBufferCacheMaxLimit;
    static std::atomic<uint64_t> DataBufferCacheRetainOverLimitMs;

    NonceGen Nonce;
    std::unique_ptr<BufferedFileIo> BufferedIo;
    std::vector<DataBufferPtr> ReadyData;
  
  public:
    enum 
    { 
      MAX_SIZE_DEFAULT = 8 * 1024 * 1024,
      
      QUEUE_BUFFER_SIZE = 256 * 1024,       // buffer size
      DATA_BUFFER_CACHE_RETAIN_OVER_LIMIT_MS_DEFAULT = 0,
      DATA_BUFFER_CACHE_MAX_LIMIT_DEFAULT = 0,
      MAX_TOTAL_BUFFERS = 128,              // hard limit for queue buffers
      FLUSH_PRESSURE_BUFFERS = 96,          // force processing if used buffers >= limit
      QUEUE_SIZE_LIMIT = QUEUE_BUFFER_SIZE * FLUSH_PRESSURE_BUFFERS,
      STAT_OUTPUT_PERIOD = 10 * 60 * 1000,  // 10 min

      RIGHT_NOW = 1,                        // Force flush right now
      FLUSH_AFTER_DEFAULT = 500,
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
    LOGMELNK bool IsAsyncSupported() const override;

    LOGMELNK static size_t GetMaxSizeDefault();
    LOGMELNK static void SetMaxSizeDefault(size_t size);

    LOGMELNK static size_t GetQueueSizeLimitDefault();
    LOGMELNK static void SetQueueSizeLimitDefault(size_t size);

    LOGMELNK static size_t GetQueueBufferLimitDefault();
    LOGMELNK static void SetQueueBufferLimitDefault(size_t count);

    LOGMELNK static uint64_t GetFlushAfterDefault();
    LOGMELNK static void SetFlushAfterDefault(uint64_t ms);

    LOGMELNK static size_t GetDataBufferSizeDefault();
    LOGMELNK static void SetDataBufferSizeDefault(size_t size);

    LOGMELNK static size_t GetDataBufferCacheLimit();
    LOGMELNK static void SetDataBufferCacheLimit(size_t count);

    LOGMELNK static size_t GetDataBufferCacheMaxLimit();
    LOGMELNK static void SetDataBufferCacheMaxLimit(size_t count);

    LOGMELNK static uint64_t GetDataBufferCacheRetainOverLimitMs();
    LOGMELNK static void SetDataBufferCacheRetainOverLimitMs(uint64_t ms);

    LOGMELNK static void ClearDataBufferCache();

    using FileIo::Truncate;

    LOGMELNK bool TestFileInUse(const std::string& file) const;
    LOGMELNK size_t GetSize();

    LOGMELNK uint64_t GetFlushTime() const;
    LOGMELNK bool HasEvents() const;
    LOGMELNK void AppendString(const char* text, size_t len);
    LOGMELNK void RegisterAsync();
    LOGMELNK static FileBackendCounters GetCounters();

  private:
    static DataBufferPtr TakeCachedDataBuffer(
      void* context
      , MemoryUsageTracker* memoryTracker
      , std::size_t capacity
    );
    static bool ReturnCachedDataBuffer(void* context, DataBufferPtr buffer);
    static void CountDataBufferAllocation(void* context);
  
  public:
  
  protected:
    LOGMELNK void Display(Context& context) override;
    void AppendStringInternal(const char* text, size_t len);

  private:
    class FileManagerFactory& GetFactory() const;
    class CompressionManagerFactory& GetCompressionFactory() const;
    FileIo& GetActiveIo();
    const FileIo& GetActiveIo() const;
    bool IsLogOpen() const;

    void SubmitCompletedFile(const std::string& file);
    std::regex BuildCleanPattern() const;
    void ApplyRetention();
    bool CompleteCurrentFile(
      FileCompletionReason reason
      , bool applyRetention = true
      , std::time_t completedArchiveTime = 0
    );
    bool ApplySizeLimit(size_t add);
    void Truncate();
    void AppendObfuscated(const char* text, size_t add);
    void AppendOutputData(const char* text, size_t add);
    enum class FlushRequestSource
    {
      UNKNOWN,
      FLUSH,
      FREEZE,
      PRESSURE_BUFFERS,
      PRESSURE_BYTES,
      PRESSURE_BOTH,
      FIRST_DATA,
    };

    enum class PublishCurrentSource
    {
      FLUSH,
      WORKER_IMMEDIATE,
      WORKER_SHUTDOWN,
      ON_SHUTDOWN,
    };

    void RequestFlush(
      uint64_t when = RIGHT_NOW
      , FlushRequestSource source = FlushRequestSource::UNKNOWN
    );
    bool PublishCurrentCounted(
      bool& needSignal
      , PublishCurrentSource source
    );
    bool WriteReadyData(std::vector<DataBufferPtr>& data);
    void UpdateFlushTimeAfterWork();

    void WaitForShutdown();


    friend class FileManager;
    bool WorkerFunc();
    void OnShutdown();

    bool ChangePart(bool applyRetention = true);
  };

  typedef std::shared_ptr<class FileBackend> FileBackendPtr;
}
