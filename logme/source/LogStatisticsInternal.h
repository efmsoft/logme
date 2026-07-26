#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <Logme/LogStatistics.h>
#include <Logme/Types.h>

namespace Logme
{
  struct Backend;
  class Channel;
  class LogStatisticsCollector;
  struct Context;
  struct ContextCache;

  uintptr_t* GetCLogStatisticsCache(Context& context);

  struct LogSiteChannelStatistics
  {
    Channel* ChannelKey;
    std::string ChannelName;
    std::atomic<uint64_t> Records;
    std::atomic<uint64_t> MessageBytes;
    std::atomic<uint64_t> MaxMessageBytes;

    LogSiteChannelStatistics(
      Channel* channel
      , const std::string& channelName
    );

    void Reset();
    void Record(size_t messageBytes);
  };

  struct LogSiteStatistics
  {
    enum
    {
      CHANNEL_SLOT_COUNT = 4,
      BACKEND_SLOT_COUNT = 8
    };

    const void* SiteKey;
    std::string File;
    std::string Method;
    std::string Format;
    int Line;
    Level ErrorLevel;

    std::atomic<LogSiteChannelStatistics*> ChannelSlots[CHANNEL_SLOT_COUNT];
    std::atomic<uint64_t> OverflowRecords;
    std::atomic<uint64_t> OverflowMessageBytes;
    std::atomic<uint64_t> OverflowMaxMessageBytes;

    struct BackendStatistics
    {
      uint64_t BackendId;
      std::string ChannelName;
      std::string BackendType;
      std::atomic<uint64_t> Records;
      std::atomic<uint64_t> OutputBytes;
      std::atomic<uint64_t> MaxOutputBytes;

      BackendStatistics(
        uint64_t backendId
        , const std::string& channelName
        , const std::string& backendType
      );

      void Reset();
      void Record(size_t outputBytes);
    };

    std::atomic<BackendStatistics*> BackendSlots[BACKEND_SLOT_COUNT];
    std::atomic<uint64_t> OverflowBackendRecords;
    std::atomic<uint64_t> OverflowBackendBytes;
    std::atomic<uint64_t> OverflowBackendMaxBytes;

    std::vector<std::unique_ptr<LogSiteChannelStatistics>> OwnedChannels;
    std::vector<std::unique_ptr<BackendStatistics>> OwnedBackends;

    LogSiteStatistics(
      const void* siteKey
      , const Context& context
      , const char* format
    );

    void Reset();
  };

  struct BackendRuntimeStatistics
  {
    uint64_t BackendId;
    std::string ChannelName;
    std::string BackendType;
    std::string Details;
    bool Async;

    std::atomic<uint64_t> AcceptedRecords;
    std::atomic<uint64_t> AcceptedBytes;
    std::atomic<uint64_t> QueueDroppedRecords;
    std::atomic<uint64_t> QueueDroppedBytes;
    std::atomic<uint64_t> WriteBatches;
    std::atomic<uint64_t> WriteOperations;
    std::atomic<uint64_t> BatchBuffers;
    std::atomic<uint64_t> BatchBytes;
    std::atomic<uint64_t> MaxBatchBuffers;
    std::atomic<uint64_t> MaxBatchBytes;
    std::atomic<uint64_t> WrittenBuffers;
    std::atomic<uint64_t> WrittenBytes;
    std::atomic<uint64_t> FailedBatches;
    std::atomic<uint64_t> FailedWriteOperations;
    std::atomic<uint64_t> FailedBuffers;
    std::atomic<uint64_t> FailedBytes;

    BackendRuntimeStatistics(
      Backend* backend
      , Channel* channel
    );

    void Reset();
    void RecordAccepted(size_t outputBytes);
    void RecordQueueDrop(size_t outputBytes);
    void RecordWrite(
      size_t batchBuffers
      , size_t batchBytes
      , size_t writeOperations
      , size_t writtenBuffers
      , size_t writtenBytes
      , size_t failedWriteOperations
      , size_t failedBuffers
      , size_t failedBytes
    );
  };

  class LogStatisticsCollector
  {
    struct EntrySnapshot
    {
      std::string File;
      std::string Method;
      std::string Format;
      std::string Channel;
      int Line = 0;
      Level ErrorLevel = Level::LEVEL_DEBUG;
      uint64_t Records = 0;
      uint64_t MessageBytes = 0;
      uint64_t MaxMessageBytes = 0;
    };

    struct ChannelSnapshot
    {
      std::string Channel;
      uint64_t Records = 0;
      uint64_t MessageBytes = 0;
      uint64_t MaxMessageBytes = 0;
    };

    struct BackendEntrySnapshot
    {
      std::string File;
      std::string Method;
      std::string Format;
      std::string Channel;
      std::string BackendType;
      int Line = 0;
      Level ErrorLevel = Level::LEVEL_DEBUG;
      uint64_t Records = 0;
      uint64_t OutputBytes = 0;
      uint64_t MaxOutputBytes = 0;
    };

    struct BackendSnapshot
    {
      std::string Channel;
      std::string BackendType;
      uint64_t Records = 0;
      uint64_t OutputBytes = 0;
      uint64_t MaxOutputBytes = 0;
    };

    struct FileBackendSnapshot
    {
      uint64_t BackendId = 0;
      std::string Channel;
      std::string BackendType;
      std::string Details;
      bool Async = false;
      uint64_t AcceptedRecords = 0;
      uint64_t AcceptedBytes = 0;
      uint64_t QueueDroppedRecords = 0;
      uint64_t QueueDroppedBytes = 0;
      uint64_t WriteBatches = 0;
      uint64_t WriteOperations = 0;
      uint64_t BatchBuffers = 0;
      uint64_t BatchBytes = 0;
      uint64_t MaxBatchBuffers = 0;
      uint64_t MaxBatchBytes = 0;
      uint64_t WrittenBuffers = 0;
      uint64_t WrittenBytes = 0;
      uint64_t FailedBatches = 0;
      uint64_t FailedWriteOperations = 0;
      uint64_t FailedBuffers = 0;
      uint64_t FailedBytes = 0;
    };

    const uint64_t Generation;
    mutable std::mutex Lock;
    std::unordered_map<const void*, LogSiteStatistics*> SiteByKey;
    std::vector<std::unique_ptr<LogSiteStatistics>> Sites;
    std::unordered_map<uint64_t, BackendRuntimeStatistics*> RuntimeBackendById;
    std::vector<std::unique_ptr<BackendRuntimeStatistics>> RuntimeBackends;

    std::atomic<uint64_t> InFlight;
    std::chrono::steady_clock::time_point StartedAt;
    std::chrono::steady_clock::time_point StoppedAt;
    bool HasSession;

  public:
    LogStatisticsCollector();

    uint64_t GetGeneration() const;

    void Enter();
    void Leave();
    void WaitForIdle() const;

    void Start();
    void Stop();
    void Reset();

    void Record(
      Context& context
      , Channel* channel
      , const char* format
      , size_t messageBytes
    );

    void RecordBackend(
      LogSiteStatistics* site
      , Channel* channel
      , Backend* backend
      , size_t outputBytes
    );

    void RecordFileBackendQueueDrop(
      Channel* channel
      , Backend* backend
      , size_t outputBytes
    );

    void RecordFileBackendWrite(
      Channel* channel
      , Backend* backend
      , size_t batchBuffers
      , size_t batchBytes
      , size_t writeOperations
      , size_t writtenBuffers
      , size_t writtenBytes
      , size_t failedWriteOperations
      , size_t failedBuffers
      , size_t failedBytes
    );

    std::string FormatStatus(bool active) const;

    std::string FormatTop(
      bool active
      , LogStatisticsSort sort
      , size_t limit
    ) const;

    std::string FormatChannels(
      bool active
      , LogStatisticsSort sort
      , size_t limit
    ) const;

    std::string FormatOutputs(
      bool active
      , LogStatisticsSort sort
      , size_t limit
      , const std::string& backendType
    ) const;

    std::string FormatBackends(
      bool active
      , LogStatisticsSort sort
      , size_t limit
      , const std::string& backendType
    ) const;

    std::string FormatFileBackends(
      bool active
      , LogFileStatisticsSort sort
      , size_t limit
    ) const;

  private:
    LogSiteStatistics* GetOrCreateSite(
      Context& context
      , const char* format
    );

    LogSiteChannelStatistics* GetOrCreateChannel(
      LogSiteStatistics& site
      , Channel* channel
    );

    LogSiteStatistics::BackendStatistics* GetOrCreateBackend(
      LogSiteStatistics& site
      , Channel* channel
      , Backend* backend
    );

    BackendRuntimeStatistics* GetOrCreateRuntimeBackend(
      Channel* channel
      , Backend* backend
    );

    std::vector<EntrySnapshot> SnapshotEntries() const;
    std::vector<ChannelSnapshot> SnapshotChannels() const;
    std::vector<BackendEntrySnapshot> SnapshotBackendEntries(
      const std::string& backendType
    ) const;
    std::vector<BackendSnapshot> SnapshotBackends(
      const std::string& backendType
    ) const;
    std::vector<FileBackendSnapshot> SnapshotFileBackends() const;
    double GetDurationSeconds(bool active) const;
  };

}
