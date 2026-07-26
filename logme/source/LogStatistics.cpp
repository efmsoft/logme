#include "LogStatisticsInternal.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <thread>

#include <Logme/Backend/Backend.h>
#include <Logme/Backend/FileBackend.h>
#include <Logme/Channel.h>
#include <Logme/Context.h>
#include <Logme/Logger.h>
#include <Logme/Utils.h>

using namespace Logme;

namespace
{
  std::atomic<uint64_t> NextGeneration(1);

  const void* GetSiteKey(Context& context)
  {
    uintptr_t* externalCache = GetCLogStatisticsCache(context);
    if (externalCache != nullptr)
      return externalCache;

    return &context.Cache;
  }

  LogSiteStatistics* LoadCachedSite(
    Context& context
    , uint64_t generation
  )
  {
    uintptr_t* externalCache = GetCLogStatisticsCache(context);
    if (externalCache == nullptr)
    {
      if (context.Cache.StatisticsGeneration.load(std::memory_order_acquire)
        != generation)
      {
        return nullptr;
      }

      return context.Cache.Statistics.load(std::memory_order_relaxed);
    }

    std::atomic_ref<uintptr_t> generationReference(
      externalCache[0]
    );

    if (generationReference.load(std::memory_order_acquire)
      != static_cast<uintptr_t>(generation))
    {
      return nullptr;
    }

    std::atomic_ref<uintptr_t> statisticsReference(
      externalCache[1]
    );

    return reinterpret_cast<LogSiteStatistics*>(
      statisticsReference.load(std::memory_order_relaxed)
    );
  }

  void StoreCachedSite(
    Context& context
    , uint64_t generation
    , LogSiteStatistics* site
  )
  {
    uintptr_t* externalCache = GetCLogStatisticsCache(context);
    if (externalCache == nullptr)
    {
      context.Cache.Statistics.store(site, std::memory_order_relaxed);
      context.Cache.StatisticsGeneration.store(
        generation
        , std::memory_order_release
      );
      return;
    }

    std::atomic_ref<uintptr_t> statisticsReference(
      externalCache[1]
    );
    statisticsReference.store(
      reinterpret_cast<uintptr_t>(site)
      , std::memory_order_relaxed
    );

    std::atomic_ref<uintptr_t> generationReference(
      externalCache[0]
    );
    generationReference.store(
      static_cast<uintptr_t>(generation)
      , std::memory_order_release
    );
  }

  void UpdateMaximum(
    std::atomic<uint64_t>& target
    , uint64_t value
  )
  {
    uint64_t current = target.load(std::memory_order_relaxed);
    while (current < value
      && !target.compare_exchange_weak(
        current
        , value
        , std::memory_order_relaxed
        , std::memory_order_relaxed
      ))
    {
    }
  }

  std::string CopyMetadata(
    const char* text
    , size_t maximumLength
  )
  {
    if (text == nullptr)
      return std::string();

    std::string result(text);
    if (result.size() > maximumLength)
    {
      result.resize(maximumLength);
      result += "...";
    }

    return result;
  }

  const char* GetSortName(LogStatisticsSort sort)
  {
    return sort == LogStatisticsSort::RECORDS ? "records" : "message-bytes";
  }

  const char* GetFileSortName(LogFileStatisticsSort sort)
  {
    switch (sort)
    {
      case LogFileStatisticsSort::WRITE_BATCHES:
        return "write-batches";

      case LogFileStatisticsSort::WRITE_ERRORS:
        return "write-errors";

      case LogFileStatisticsSort::DROPPED_BYTES:
        return "dropped-bytes";

      case LogFileStatisticsSort::WRITTEN_BYTES:
      default:
        return "written-bytes";
    }
  }

  double GetRate(
    uint64_t value
    , double seconds
  )
  {
    if (seconds <= 0.0)
      return 0.0;

    return static_cast<double>(value) / seconds;
  }

  double GetAverage(
    uint64_t bytes
    , uint64_t records
  )
  {
    if (records == 0)
      return 0.0;

    return static_cast<double>(bytes) / static_cast<double>(records);
  }

  double GetShare(
    uint64_t value
    , uint64_t total
  )
  {
    if (total == 0)
      return 0.0;

    return 100.0
      * static_cast<double>(value)
      / static_cast<double>(total);
  }

  void AppendEntryHeader(
    std::string& response
    , bool active
    , double duration
    , LogStatisticsSort sort
    , uint64_t records
    , uint64_t bytes
  )
  {
    char line[512];
    snprintf(
      line
      , sizeof(line)
      , "Log statistics: %s\nDuration: %.3f s\nSort: %s\nTotal: records=%llu message-bytes=%llu\n"
      , active ? "running" : "stopped"
      , duration
      , GetSortName(sort)
      , static_cast<unsigned long long>(records)
      , static_cast<unsigned long long>(bytes)
    );
    response += line;
  }

  void AppendOutputHeader(
    std::string& response
    , bool active
    , double duration
    , LogStatisticsSort sort
    , uint64_t records
    , uint64_t bytes
    , const std::string& backendType
  )
  {
    char line[640];
    snprintf(
      line
      , sizeof(line)
      , "Log backend statistics: %s\nDuration: %.3f s\nSort: %s\nBackend filter: %s\nTotal: records=%llu output-bytes=%llu\n"
      , active ? "running" : "stopped"
      , duration
      , sort == LogStatisticsSort::RECORDS ? "records" : "output-bytes"
      , backendType.empty() ? "<all>" : backendType.c_str()
      , static_cast<unsigned long long>(records)
      , static_cast<unsigned long long>(bytes)
    );
    response += line;
  }
}

LogSiteChannelStatistics::LogSiteChannelStatistics(
  Channel* channel
  , const std::string& channelName
)
  : ChannelKey(channel)
  , ChannelName(channelName)
  , Records(0)
  , MessageBytes(0)
  , MaxMessageBytes(0)
{
}

void LogSiteChannelStatistics::Reset()
{
  Records.store(0, std::memory_order_relaxed);
  MessageBytes.store(0, std::memory_order_relaxed);
  MaxMessageBytes.store(0, std::memory_order_relaxed);
}

void LogSiteChannelStatistics::Record(size_t messageBytes)
{
  Records.fetch_add(1, std::memory_order_relaxed);
  MessageBytes.fetch_add(messageBytes, std::memory_order_relaxed);
  UpdateMaximum(MaxMessageBytes, messageBytes);
}

LogSiteStatistics::LogSiteStatistics(
  const void* siteKey
  , const Context& context
  , const char* format
)
  : SiteKey(siteKey)
  , File(CopyMetadata(context.File.FullName, 1024))
  , Method(CopyMetadata(context.Method, 512))
  , Format(CopyMetadata(format, 1024))
  , Line(context.Line)
  , ErrorLevel(context.ErrorLevel)
  , OverflowRecords(0)
  , OverflowMessageBytes(0)
  , OverflowMaxMessageBytes(0)
  , OverflowBackendRecords(0)
  , OverflowBackendBytes(0)
  , OverflowBackendMaxBytes(0)
{
  for (auto& slot : ChannelSlots)
    slot.store(nullptr, std::memory_order_relaxed);

  for (auto& slot : BackendSlots)
    slot.store(nullptr, std::memory_order_relaxed);
}

void LogSiteStatistics::Reset()
{
  OverflowRecords.store(0, std::memory_order_relaxed);
  OverflowMessageBytes.store(0, std::memory_order_relaxed);
  OverflowMaxMessageBytes.store(0, std::memory_order_relaxed);
  OverflowBackendRecords.store(0, std::memory_order_relaxed);
  OverflowBackendBytes.store(0, std::memory_order_relaxed);
  OverflowBackendMaxBytes.store(0, std::memory_order_relaxed);

  for (auto& channel : OwnedChannels)
    channel->Reset();

  for (auto& backend : OwnedBackends)
    backend->Reset();
}

LogSiteStatistics::BackendStatistics::BackendStatistics(
  uint64_t backendId
  , const std::string& channelName
  , const std::string& backendType
)
  : BackendId(backendId)
  , ChannelName(channelName)
  , BackendType(backendType)
  , Records(0)
  , OutputBytes(0)
  , MaxOutputBytes(0)
{
}

void LogSiteStatistics::BackendStatistics::Reset()
{
  Records.store(0, std::memory_order_relaxed);
  OutputBytes.store(0, std::memory_order_relaxed);
  MaxOutputBytes.store(0, std::memory_order_relaxed);
}

void LogSiteStatistics::BackendStatistics::Record(size_t outputBytes)
{
  Records.fetch_add(1, std::memory_order_relaxed);
  OutputBytes.fetch_add(outputBytes, std::memory_order_relaxed);
  UpdateMaximum(MaxOutputBytes, outputBytes);
}

BackendRuntimeStatistics::BackendRuntimeStatistics(
  Backend* backend
  , Channel* channel
)
  : BackendId(backend->GetStatisticsId())
  , ChannelName(channel != nullptr ? channel->GetName() : "<default>")
  , BackendType(backend->GetType() != nullptr ? backend->GetType() : "<unknown>")
  , Details(backend->FormatDetails())
  , Async(backend->GetAsync())
  , AcceptedRecords(0)
  , AcceptedBytes(0)
  , QueueDroppedRecords(0)
  , QueueDroppedBytes(0)
  , WriteBatches(0)
  , WriteOperations(0)
  , BatchBuffers(0)
  , BatchBytes(0)
  , MaxBatchBuffers(0)
  , MaxBatchBytes(0)
  , WrittenBuffers(0)
  , WrittenBytes(0)
  , FailedBatches(0)
  , FailedWriteOperations(0)
  , FailedBuffers(0)
  , FailedBytes(0)
{
  if (ChannelName.empty())
    ChannelName = "<default>";

  if (BackendType.empty())
    BackendType = "<unknown>";
}

void BackendRuntimeStatistics::Reset()
{
  AcceptedRecords.store(0, std::memory_order_relaxed);
  AcceptedBytes.store(0, std::memory_order_relaxed);
  QueueDroppedRecords.store(0, std::memory_order_relaxed);
  QueueDroppedBytes.store(0, std::memory_order_relaxed);
  WriteBatches.store(0, std::memory_order_relaxed);
  WriteOperations.store(0, std::memory_order_relaxed);
  BatchBuffers.store(0, std::memory_order_relaxed);
  BatchBytes.store(0, std::memory_order_relaxed);
  MaxBatchBuffers.store(0, std::memory_order_relaxed);
  MaxBatchBytes.store(0, std::memory_order_relaxed);
  WrittenBuffers.store(0, std::memory_order_relaxed);
  WrittenBytes.store(0, std::memory_order_relaxed);
  FailedBatches.store(0, std::memory_order_relaxed);
  FailedWriteOperations.store(0, std::memory_order_relaxed);
  FailedBuffers.store(0, std::memory_order_relaxed);
  FailedBytes.store(0, std::memory_order_relaxed);
}

void BackendRuntimeStatistics::RecordAccepted(size_t outputBytes)
{
  AcceptedRecords.fetch_add(1, std::memory_order_relaxed);
  AcceptedBytes.fetch_add(outputBytes, std::memory_order_relaxed);
}

void BackendRuntimeStatistics::RecordQueueDrop(size_t outputBytes)
{
  QueueDroppedRecords.fetch_add(1, std::memory_order_relaxed);
  QueueDroppedBytes.fetch_add(outputBytes, std::memory_order_relaxed);
}

void BackendRuntimeStatistics::RecordWrite(
  size_t batchBuffers
  , size_t batchBytes
  , size_t writeOperations
  , size_t writtenBuffers
  , size_t writtenBytes
  , size_t failedWriteOperations
  , size_t failedBuffers
  , size_t failedBytes
)
{
  WriteBatches.fetch_add(1, std::memory_order_relaxed);
  WriteOperations.fetch_add(writeOperations, std::memory_order_relaxed);
  BatchBuffers.fetch_add(batchBuffers, std::memory_order_relaxed);
  BatchBytes.fetch_add(batchBytes, std::memory_order_relaxed);
  UpdateMaximum(MaxBatchBuffers, batchBuffers);
  UpdateMaximum(MaxBatchBytes, batchBytes);
  WrittenBuffers.fetch_add(writtenBuffers, std::memory_order_relaxed);
  WrittenBytes.fetch_add(writtenBytes, std::memory_order_relaxed);

  if (failedWriteOperations != 0 || failedBuffers != 0 || failedBytes != 0)
    FailedBatches.fetch_add(1, std::memory_order_relaxed);

  FailedWriteOperations.fetch_add(
    failedWriteOperations
    , std::memory_order_relaxed
  );
  FailedBuffers.fetch_add(failedBuffers, std::memory_order_relaxed);
  FailedBytes.fetch_add(failedBytes, std::memory_order_relaxed);
}

LogStatisticsCollector::LogStatisticsCollector()
  : Generation(NextGeneration.fetch_add(1, std::memory_order_relaxed))
  , InFlight(0)
  , HasSession(false)
{
}

uint64_t LogStatisticsCollector::GetGeneration() const
{
  return Generation;
}

void LogStatisticsCollector::Enter()
{
  InFlight.fetch_add(1, std::memory_order_acquire);
}

void LogStatisticsCollector::Leave()
{
  InFlight.fetch_sub(1, std::memory_order_release);
}

void LogStatisticsCollector::WaitForIdle() const
{
  while (InFlight.load(std::memory_order_acquire) != 0)
    std::this_thread::yield();
}

void LogStatisticsCollector::Start()
{
  Reset();

  StartedAt = std::chrono::steady_clock::now();
  StoppedAt = StartedAt;
  HasSession = true;
}

void LogStatisticsCollector::Stop()
{
  if (!HasSession)
    return;

  StoppedAt = std::chrono::steady_clock::now();
}

void LogStatisticsCollector::Reset()
{
  std::lock_guard guard(Lock);

  for (auto& site : Sites)
    site->Reset();

  for (auto& backend : RuntimeBackends)
    backend->Reset();

  if (HasSession)
  {
    StartedAt = std::chrono::steady_clock::now();
    StoppedAt = StartedAt;
  }
}

void LogStatisticsCollector::Record(
  Context& context
  , Channel* channel
  , const char* format
  , size_t messageBytes
)
{
  LogSiteStatistics* site = LoadCachedSite(context, Generation);

  if (site == nullptr)
  {
    site = GetOrCreateSite(context, format);
    StoreCachedSite(context, Generation, site);
  }

  for (auto& slot : site->ChannelSlots)
  {
    LogSiteChannelStatistics* channelStatistics = slot.load(std::memory_order_acquire);
    if (channelStatistics == nullptr)
      break;

    if (channelStatistics->ChannelKey == channel)
    {
      channelStatistics->Record(messageBytes);
      return;
    }
  }

  LogSiteChannelStatistics* channelStatistics = GetOrCreateChannel(*site, channel);
  if (channelStatistics != nullptr)
  {
    channelStatistics->Record(messageBytes);
    return;
  }

  site->OverflowRecords.fetch_add(1, std::memory_order_relaxed);
  site->OverflowMessageBytes.fetch_add(messageBytes, std::memory_order_relaxed);
  UpdateMaximum(site->OverflowMaxMessageBytes, messageBytes);
  return;
}

void LogStatisticsCollector::RecordBackend(
  LogSiteStatistics* site
  , Channel* channel
  , Backend* backend
  , size_t outputBytes
)
{
  if (site == nullptr || backend == nullptr)
    return;

  const char* backendType = backend->GetType();
  if (backend->GetAsync()
    && backendType != nullptr
    && std::strcmp(backendType, FileBackend::TYPE_ID) == 0)
  {
    BackendRuntimeStatistics* runtime = GetOrCreateRuntimeBackend(
      channel
      , backend
    );

    if (runtime != nullptr)
      runtime->RecordAccepted(outputBytes);
  }

  uint64_t backendId = backend->GetStatisticsId();
  for (auto& slot : site->BackendSlots)
  {
    LogSiteStatistics::BackendStatistics* statistics = slot.load(
      std::memory_order_acquire
    );

    if (statistics == nullptr)
      break;

    if (statistics->BackendId == backendId)
    {
      statistics->Record(outputBytes);
      return;
    }
  }

  LogSiteStatistics::BackendStatistics* statistics = GetOrCreateBackend(
    *site
    , channel
    , backend
  );

  if (statistics != nullptr)
  {
    statistics->Record(outputBytes);
    return;
  }

  site->OverflowBackendRecords.fetch_add(1, std::memory_order_relaxed);
  site->OverflowBackendBytes.fetch_add(outputBytes, std::memory_order_relaxed);
  UpdateMaximum(site->OverflowBackendMaxBytes, outputBytes);
}

void LogStatisticsCollector::RecordFileBackendQueueDrop(
  Channel* channel
  , Backend* backend
  , size_t outputBytes
)
{
  if (backend == nullptr)
    return;

  BackendRuntimeStatistics* runtime = GetOrCreateRuntimeBackend(
    channel
    , backend
  );

  if (runtime != nullptr)
    runtime->RecordQueueDrop(outputBytes);
}

void LogStatisticsCollector::RecordFileBackendWrite(
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
)
{
  if (backend == nullptr)
    return;

  BackendRuntimeStatistics* runtime = GetOrCreateRuntimeBackend(
    channel
    , backend
  );

  if (runtime == nullptr)
    return;

  runtime->RecordWrite(
    batchBuffers
    , batchBytes
    , writeOperations
    , writtenBuffers
    , writtenBytes
    , failedWriteOperations
    , failedBuffers
    , failedBytes
  );
}

LogSiteStatistics* LogStatisticsCollector::GetOrCreateSite(
  Context& context
  , const char* format
)
{
  std::lock_guard guard(Lock);

  const void* siteKey = GetSiteKey(context);

  auto it = SiteByKey.find(siteKey);
  if (it != SiteByKey.end())
  {
    LogSiteStatistics* existing = it->second;
    const char* file = context.File.FullName ? context.File.FullName : "";
    const char* method = context.Method ? context.Method : "";

    if (existing->Line == context.Line
      && existing->File == file
      && existing->Method == method)
    {
      return existing;
    }
  }

  auto site = std::make_unique<LogSiteStatistics>(
    siteKey
    , context
    , format
  );

  LogSiteStatistics* result = site.get();
  Sites.push_back(std::move(site));
  SiteByKey[siteKey] = result;
  return result;
}

LogSiteChannelStatistics* LogStatisticsCollector::GetOrCreateChannel(
  LogSiteStatistics& site
  , Channel* channel
)
{
  std::string channelName = channel ? channel->GetName() : std::string("<none>");
  if (channelName.empty())
    channelName = "<default>";

  std::lock_guard guard(Lock);

  for (auto& slot : site.ChannelSlots)
  {
    LogSiteChannelStatistics* current = slot.load(std::memory_order_relaxed);
    if (current != nullptr && current->ChannelKey == channel)
      return current;
  }

  for (auto& slot : site.ChannelSlots)
  {
    if (slot.load(std::memory_order_relaxed) != nullptr)
      continue;

    auto channelStatistics = std::make_unique<LogSiteChannelStatistics>(
      channel
      , channelName
    );

    LogSiteChannelStatistics* result = channelStatistics.get();
    site.OwnedChannels.push_back(std::move(channelStatistics));
    slot.store(result, std::memory_order_release);
    return result;
  }

  return nullptr;
}

LogSiteStatistics::BackendStatistics* LogStatisticsCollector::GetOrCreateBackend(
  LogSiteStatistics& site
  , Channel* channel
  , Backend* backend
)
{
  uint64_t backendId = backend->GetStatisticsId();
  std::string channelName = channel ? channel->GetName() : std::string("<none>");
  if (channelName.empty())
    channelName = "<default>";

  const char* type = backend->GetType();
  std::string backendType = type && *type ? type : "<unknown>";

  std::lock_guard guard(Lock);

  for (auto& slot : site.BackendSlots)
  {
    LogSiteStatistics::BackendStatistics* current = slot.load(
      std::memory_order_relaxed
    );

    if (current != nullptr && current->BackendId == backendId)
      return current;
  }

  for (auto& slot : site.BackendSlots)
  {
    if (slot.load(std::memory_order_relaxed) != nullptr)
      continue;

    auto statistics = std::make_unique<LogSiteStatistics::BackendStatistics>(
      backendId
      , channelName
      , backendType
    );

    LogSiteStatistics::BackendStatistics* result = statistics.get();
    site.OwnedBackends.push_back(std::move(statistics));
    slot.store(result, std::memory_order_release);
    return result;
  }

  return nullptr;
}

BackendRuntimeStatistics* LogStatisticsCollector::GetOrCreateRuntimeBackend(
  Channel* channel
  , Backend* backend
)
{
  const char* backendType = backend->GetType();
  if (backendType == nullptr
    || std::strcmp(backendType, FileBackend::TYPE_ID) != 0)
  {
    return nullptr;
  }

  FileBackend* fileBackend = static_cast<FileBackend*>(backend);

  if (fileBackend->RuntimeStatisticsGeneration.load(
      std::memory_order_acquire
    ) == Generation)
  {
    BackendRuntimeStatistics* cached = fileBackend->RuntimeStatistics.load(
      std::memory_order_relaxed
    );

    if (cached != nullptr)
      return cached;
  }

  std::lock_guard guard(Lock);

  uint64_t backendId = backend->GetStatisticsId();
  auto it = RuntimeBackendById.find(backendId);
  if (it != RuntimeBackendById.end())
  {
    fileBackend->RuntimeStatistics.store(
      it->second
      , std::memory_order_relaxed
    );
    fileBackend->RuntimeStatisticsGeneration.store(
      Generation
      , std::memory_order_release
    );
    return it->second;
  }

  auto statistics = std::make_unique<BackendRuntimeStatistics>(
    backend
    , channel
  );

  BackendRuntimeStatistics* result = statistics.get();
  RuntimeBackends.push_back(std::move(statistics));
  RuntimeBackendById[backendId] = result;
  fileBackend->RuntimeStatistics.store(result, std::memory_order_relaxed);
  fileBackend->RuntimeStatisticsGeneration.store(
    Generation
    , std::memory_order_release
  );
  return result;
}

std::vector<LogStatisticsCollector::EntrySnapshot>
LogStatisticsCollector::SnapshotEntries() const
{
  std::lock_guard guard(Lock);

  std::vector<EntrySnapshot> result;
  result.reserve(Sites.size());

  for (const auto& site : Sites)
  {
    for (const auto& slot : site->ChannelSlots)
    {
      LogSiteChannelStatistics* channel = slot.load(std::memory_order_acquire);
      if (channel == nullptr)
        break;

      uint64_t records = channel->Records.load(std::memory_order_relaxed);
      if (records == 0)
        continue;

      EntrySnapshot entry;
      entry.File = site->File;
      entry.Method = site->Method;
      entry.Format = site->Format;
      entry.Channel = channel->ChannelName;
      entry.Line = site->Line;
      entry.ErrorLevel = site->ErrorLevel;
      entry.Records = records;
      entry.MessageBytes = channel->MessageBytes.load(std::memory_order_relaxed);
      entry.MaxMessageBytes = channel->MaxMessageBytes.load(std::memory_order_relaxed);
      result.push_back(std::move(entry));
    }

    uint64_t overflowRecords = site->OverflowRecords.load(std::memory_order_relaxed);
    if (overflowRecords != 0)
    {
      EntrySnapshot entry;
      entry.File = site->File;
      entry.Method = site->Method;
      entry.Format = site->Format;
      entry.Channel = "<other channels>";
      entry.Line = site->Line;
      entry.ErrorLevel = site->ErrorLevel;
      entry.Records = overflowRecords;
      entry.MessageBytes = site->OverflowMessageBytes.load(std::memory_order_relaxed);
      entry.MaxMessageBytes = site->OverflowMaxMessageBytes.load(std::memory_order_relaxed);
      result.push_back(std::move(entry));
    }
  }

  return result;
}

std::vector<LogStatisticsCollector::ChannelSnapshot>
LogStatisticsCollector::SnapshotChannels() const
{
  std::vector<EntrySnapshot> entries = SnapshotEntries();
  std::unordered_map<std::string, ChannelSnapshot> channels;

  for (const EntrySnapshot& entry : entries)
  {
    ChannelSnapshot& channel = channels[entry.Channel];
    channel.Channel = entry.Channel;
    channel.Records += entry.Records;
    channel.MessageBytes += entry.MessageBytes;
    channel.MaxMessageBytes = std::max(
      channel.MaxMessageBytes
      , entry.MaxMessageBytes
    );
  }

  std::vector<ChannelSnapshot> result;
  result.reserve(channels.size());
  for (auto& item : channels)
    result.push_back(std::move(item.second));

  return result;
}

std::vector<LogStatisticsCollector::BackendEntrySnapshot>
LogStatisticsCollector::SnapshotBackendEntries(
  const std::string& backendType
) const
{
  std::lock_guard guard(Lock);

  std::vector<BackendEntrySnapshot> result;
  result.reserve(Sites.size());

  for (const auto& site : Sites)
  {
    for (const auto& slot : site->BackendSlots)
    {
      LogSiteStatistics::BackendStatistics* backend = slot.load(
        std::memory_order_acquire
      );

      if (backend == nullptr)
        break;

      if (!backendType.empty() && backend->BackendType != backendType)
        continue;

      uint64_t records = backend->Records.load(std::memory_order_relaxed);
      if (records == 0)
        continue;

      BackendEntrySnapshot entry;
      entry.File = site->File;
      entry.Method = site->Method;
      entry.Format = site->Format;
      entry.Channel = backend->ChannelName;
      entry.BackendType = backend->BackendType;
      entry.Line = site->Line;
      entry.ErrorLevel = site->ErrorLevel;
      entry.Records = records;
      entry.OutputBytes = backend->OutputBytes.load(std::memory_order_relaxed);
      entry.MaxOutputBytes = backend->MaxOutputBytes.load(
        std::memory_order_relaxed
      );
      result.push_back(std::move(entry));
    }

    uint64_t overflowRecords = site->OverflowBackendRecords.load(
      std::memory_order_relaxed
    );

    if (overflowRecords != 0 && backendType.empty())
    {
      BackendEntrySnapshot entry;
      entry.File = site->File;
      entry.Method = site->Method;
      entry.Format = site->Format;
      entry.Channel = "<other destinations>";
      entry.BackendType = "<other backends>";
      entry.Line = site->Line;
      entry.ErrorLevel = site->ErrorLevel;
      entry.Records = overflowRecords;
      entry.OutputBytes = site->OverflowBackendBytes.load(
        std::memory_order_relaxed
      );
      entry.MaxOutputBytes = site->OverflowBackendMaxBytes.load(
        std::memory_order_relaxed
      );
      result.push_back(std::move(entry));
    }
  }

  return result;
}

std::vector<LogStatisticsCollector::BackendSnapshot>
LogStatisticsCollector::SnapshotBackends(
  const std::string& backendType
) const
{
  std::vector<BackendEntrySnapshot> entries = SnapshotBackendEntries(
    backendType
  );

  std::unordered_map<std::string, BackendSnapshot> backends;
  for (const BackendEntrySnapshot& entry : entries)
  {
    std::string key = entry.Channel;
    key.push_back('\0');
    key += entry.BackendType;

    BackendSnapshot& backend = backends[key];
    backend.Channel = entry.Channel;
    backend.BackendType = entry.BackendType;
    backend.Records += entry.Records;
    backend.OutputBytes += entry.OutputBytes;
    backend.MaxOutputBytes = std::max(
      backend.MaxOutputBytes
      , entry.MaxOutputBytes
    );
  }

  std::vector<BackendSnapshot> result;
  result.reserve(backends.size());
  for (auto& item : backends)
    result.push_back(std::move(item.second));

  return result;
}

std::vector<LogStatisticsCollector::FileBackendSnapshot>
LogStatisticsCollector::SnapshotFileBackends() const
{
  std::lock_guard guard(Lock);

  std::vector<FileBackendSnapshot> result;
  result.reserve(RuntimeBackends.size());

  for (const auto& runtime : RuntimeBackends)
  {
    FileBackendSnapshot snapshot;
    snapshot.BackendId = runtime->BackendId;
    snapshot.Channel = runtime->ChannelName;
    snapshot.BackendType = runtime->BackendType;
    snapshot.Details = runtime->Details;
    snapshot.Async = runtime->Async;
    snapshot.AcceptedRecords = runtime->AcceptedRecords.load(
      std::memory_order_relaxed
    );
    snapshot.AcceptedBytes = runtime->AcceptedBytes.load(
      std::memory_order_relaxed
    );
    snapshot.QueueDroppedRecords = runtime->QueueDroppedRecords.load(
      std::memory_order_relaxed
    );
    snapshot.QueueDroppedBytes = runtime->QueueDroppedBytes.load(
      std::memory_order_relaxed
    );
    snapshot.WriteBatches = runtime->WriteBatches.load(
      std::memory_order_relaxed
    );
    snapshot.WriteOperations = runtime->WriteOperations.load(
      std::memory_order_relaxed
    );
    snapshot.BatchBuffers = runtime->BatchBuffers.load(
      std::memory_order_relaxed
    );
    snapshot.BatchBytes = runtime->BatchBytes.load(
      std::memory_order_relaxed
    );
    snapshot.MaxBatchBuffers = runtime->MaxBatchBuffers.load(
      std::memory_order_relaxed
    );
    snapshot.MaxBatchBytes = runtime->MaxBatchBytes.load(
      std::memory_order_relaxed
    );
    snapshot.WrittenBuffers = runtime->WrittenBuffers.load(
      std::memory_order_relaxed
    );
    snapshot.WrittenBytes = runtime->WrittenBytes.load(
      std::memory_order_relaxed
    );
    snapshot.FailedBatches = runtime->FailedBatches.load(
      std::memory_order_relaxed
    );
    snapshot.FailedWriteOperations = runtime->FailedWriteOperations.load(
      std::memory_order_relaxed
    );
    snapshot.FailedBuffers = runtime->FailedBuffers.load(
      std::memory_order_relaxed
    );
    snapshot.FailedBytes = runtime->FailedBytes.load(
      std::memory_order_relaxed
    );

    if (snapshot.AcceptedRecords == 0
      && snapshot.QueueDroppedRecords == 0
      && snapshot.WriteBatches == 0)
    {
      continue;
    }

    result.push_back(std::move(snapshot));
  }

  return result;
}

double LogStatisticsCollector::GetDurationSeconds(bool active) const
{
  if (!HasSession)
    return 0.0;

  auto end = active ? std::chrono::steady_clock::now() : StoppedAt;
  return std::chrono::duration<double>(end - StartedAt).count();
}

std::string LogStatisticsCollector::FormatStatus(bool active) const
{
  std::vector<EntrySnapshot> entries = SnapshotEntries();
  std::vector<BackendEntrySnapshot> backendEntries = SnapshotBackendEntries(
    std::string()
  );
  std::vector<FileBackendSnapshot> fileBackends = SnapshotFileBackends();
  uint64_t records = 0;
  uint64_t bytes = 0;
  uint64_t backendRecords = 0;
  uint64_t backendBytes = 0;
  uint64_t fileWriteBatches = 0;
  uint64_t fileWrittenBytes = 0;
  uint64_t fileFailedBatches = 0;
  uint64_t fileDroppedBytes = 0;

  for (const EntrySnapshot& entry : entries)
  {
    records += entry.Records;
    bytes += entry.MessageBytes;
  }

  for (const BackendEntrySnapshot& entry : backendEntries)
  {
    backendRecords += entry.Records;
    backendBytes += entry.OutputBytes;
  }

  for (const FileBackendSnapshot& backend : fileBackends)
  {
    fileWriteBatches += backend.WriteBatches;
    fileWrittenBytes += backend.WrittenBytes;
    fileFailedBatches += backend.FailedBatches;
    fileDroppedBytes += backend.QueueDroppedBytes;
  }

  char line[768];
  snprintf(
    line
    , sizeof(line)
    , "Log statistics: %s\nSession: %s\nDuration: %.3f s\nSite/channel rows: %zu\nBackend rows: %zu\nAsync file backend rows: %zu\nRecords: %llu\nMessage bytes: %llu\nBackend records: %llu\nBackend output bytes: %llu\nFile write batches: %llu\nFile written bytes: %llu\nFile failed batches: %llu\nFile queue dropped bytes: %llu\n"
    , active ? "running" : "stopped"
    , HasSession ? "available" : "not started"
    , GetDurationSeconds(active)
    , entries.size()
    , backendEntries.size()
    , fileBackends.size()
    , static_cast<unsigned long long>(records)
    , static_cast<unsigned long long>(bytes)
    , static_cast<unsigned long long>(backendRecords)
    , static_cast<unsigned long long>(backendBytes)
    , static_cast<unsigned long long>(fileWriteBatches)
    , static_cast<unsigned long long>(fileWrittenBytes)
    , static_cast<unsigned long long>(fileFailedBatches)
    , static_cast<unsigned long long>(fileDroppedBytes)
  );

  return line;
}

std::string LogStatisticsCollector::FormatTop(
  bool active
  , LogStatisticsSort sort
  , size_t limit
) const
{
  std::vector<EntrySnapshot> entries = SnapshotEntries();

  std::sort(
    entries.begin()
    , entries.end()
    , [sort](const EntrySnapshot& left, const EntrySnapshot& right)
    {
      if (sort == LogStatisticsSort::RECORDS)
      {
        if (left.Records != right.Records)
          return left.Records > right.Records;
      }
      else if (left.MessageBytes != right.MessageBytes)
      {
        return left.MessageBytes > right.MessageBytes;
      }

      return left.File < right.File;
    }
  );

  uint64_t totalRecords = 0;
  uint64_t totalBytes = 0;
  for (const EntrySnapshot& entry : entries)
  {
    totalRecords += entry.Records;
    totalBytes += entry.MessageBytes;
  }

  double duration = GetDurationSeconds(active);
  std::string response;
  AppendEntryHeader(
    response
    , active
    , duration
    , sort
    , totalRecords
    , totalBytes
  );

  if (entries.empty())
  {
    response += "No log records collected.\n";
    return response;
  }

  size_t count = std::min(limit, entries.size());
  for (size_t i = 0; i < count; ++i)
  {
    const EntrySnapshot& entry = entries[i];
    uint64_t shareValue = sort == LogStatisticsSort::RECORDS
      ? entry.Records
      : entry.MessageBytes;
    uint64_t shareTotal = sort == LogStatisticsSort::RECORDS
      ? totalRecords
      : totalBytes;

    char line[768];
    snprintf(
      line
      , sizeof(line)
      , "\n%zu. share=%.2f%% records=%llu records/s=%.2f message-bytes=%llu KiB/s=%.2f avg=%.1f max=%llu level=%s channel=%s\n"
      , i + 1
      , GetShare(shareValue, shareTotal)
      , static_cast<unsigned long long>(entry.Records)
      , GetRate(entry.Records, duration)
      , static_cast<unsigned long long>(entry.MessageBytes)
      , GetRate(entry.MessageBytes, duration) / 1024.0
      , GetAverage(entry.MessageBytes, entry.Records)
      , static_cast<unsigned long long>(entry.MaxMessageBytes)
      , GetLevelName(entry.ErrorLevel).c_str()
      , entry.Channel.c_str()
    );
    response += line;

    response += "   ";
    response += entry.File;
    response += ":";
    response += std::to_string(entry.Line);
    response += " ";
    response += entry.Method;
    response += "\n   format: ";
    response += entry.Format.empty() ? "<stream or empty>" : entry.Format;
    response += "\n";
  }

  return response;
}

std::string LogStatisticsCollector::FormatChannels(
  bool active
  , LogStatisticsSort sort
  , size_t limit
) const
{
  std::vector<ChannelSnapshot> channels = SnapshotChannels();

  std::sort(
    channels.begin()
    , channels.end()
    , [sort](const ChannelSnapshot& left, const ChannelSnapshot& right)
    {
      if (sort == LogStatisticsSort::RECORDS)
      {
        if (left.Records != right.Records)
          return left.Records > right.Records;
      }
      else if (left.MessageBytes != right.MessageBytes)
      {
        return left.MessageBytes > right.MessageBytes;
      }

      return left.Channel < right.Channel;
    }
  );

  uint64_t totalRecords = 0;
  uint64_t totalBytes = 0;
  for (const ChannelSnapshot& channel : channels)
  {
    totalRecords += channel.Records;
    totalBytes += channel.MessageBytes;
  }

  double duration = GetDurationSeconds(active);
  std::string response;
  AppendEntryHeader(
    response
    , active
    , duration
    , sort
    , totalRecords
    , totalBytes
  );

  if (channels.empty())
  {
    response += "No log records collected.\n";
    return response;
  }

  size_t count = std::min(limit, channels.size());
  for (size_t i = 0; i < count; ++i)
  {
    const ChannelSnapshot& channel = channels[i];
    uint64_t shareValue = sort == LogStatisticsSort::RECORDS
      ? channel.Records
      : channel.MessageBytes;
    uint64_t shareTotal = sort == LogStatisticsSort::RECORDS
      ? totalRecords
      : totalBytes;

    char line[512];
    snprintf(
      line
      , sizeof(line)
      , "%zu. share=%.2f%% records=%llu records/s=%.2f message-bytes=%llu KiB/s=%.2f avg=%.1f max=%llu channel=%s\n"
      , i + 1
      , GetShare(shareValue, shareTotal)
      , static_cast<unsigned long long>(channel.Records)
      , GetRate(channel.Records, duration)
      , static_cast<unsigned long long>(channel.MessageBytes)
      , GetRate(channel.MessageBytes, duration) / 1024.0
      , GetAverage(channel.MessageBytes, channel.Records)
      , static_cast<unsigned long long>(channel.MaxMessageBytes)
      , channel.Channel.c_str()
    );
    response += line;
  }

  return response;
}

std::string LogStatisticsCollector::FormatOutputs(
  bool active
  , LogStatisticsSort sort
  , size_t limit
  , const std::string& backendType
) const
{
  std::vector<BackendEntrySnapshot> entries = SnapshotBackendEntries(
    backendType
  );

  std::sort(
    entries.begin()
    , entries.end()
    , [sort](const BackendEntrySnapshot& left, const BackendEntrySnapshot& right)
    {
      if (sort == LogStatisticsSort::RECORDS)
      {
        if (left.Records != right.Records)
          return left.Records > right.Records;
      }
      else if (left.OutputBytes != right.OutputBytes)
      {
        return left.OutputBytes > right.OutputBytes;
      }

      if (left.File != right.File)
        return left.File < right.File;

      return left.Line < right.Line;
    }
  );

  uint64_t totalRecords = 0;
  uint64_t totalBytes = 0;
  for (const BackendEntrySnapshot& entry : entries)
  {
    totalRecords += entry.Records;
    totalBytes += entry.OutputBytes;
  }

  double duration = GetDurationSeconds(active);
  std::string response;
  AppendOutputHeader(
    response
    , active
    , duration
    , sort
    , totalRecords
    , totalBytes
    , backendType
  );

  if (entries.empty())
  {
    response += "No backend output collected.\n";
    return response;
  }

  size_t count = std::min(limit, entries.size());
  for (size_t i = 0; i < count; ++i)
  {
    const BackendEntrySnapshot& entry = entries[i];
    uint64_t shareValue = sort == LogStatisticsSort::RECORDS
      ? entry.Records
      : entry.OutputBytes;
    uint64_t shareTotal = sort == LogStatisticsSort::RECORDS
      ? totalRecords
      : totalBytes;

    char line[896];
    snprintf(
      line
      , sizeof(line)
      , "\n%zu. share=%.2f%% records=%llu records/s=%.2f output-bytes=%llu KiB/s=%.2f avg=%.1f max=%llu level=%s channel=%s backend=%s\n"
      , i + 1
      , GetShare(shareValue, shareTotal)
      , static_cast<unsigned long long>(entry.Records)
      , GetRate(entry.Records, duration)
      , static_cast<unsigned long long>(entry.OutputBytes)
      , GetRate(entry.OutputBytes, duration) / 1024.0
      , GetAverage(entry.OutputBytes, entry.Records)
      , static_cast<unsigned long long>(entry.MaxOutputBytes)
      , GetLevelName(entry.ErrorLevel).c_str()
      , entry.Channel.c_str()
      , entry.BackendType.c_str()
    );
    response += line;

    response += "   ";
    response += entry.File;
    response += ":";
    response += std::to_string(entry.Line);
    response += " ";
    response += entry.Method;
    response += "\n   format: ";
    response += entry.Format.empty() ? "<stream or empty>" : entry.Format;
    response += "\n";
  }

  return response;
}

std::string LogStatisticsCollector::FormatBackends(
  bool active
  , LogStatisticsSort sort
  , size_t limit
  , const std::string& backendType
) const
{
  std::vector<BackendSnapshot> backends = SnapshotBackends(backendType);

  std::sort(
    backends.begin()
    , backends.end()
    , [sort](const BackendSnapshot& left, const BackendSnapshot& right)
    {
      if (sort == LogStatisticsSort::RECORDS)
      {
        if (left.Records != right.Records)
          return left.Records > right.Records;
      }
      else if (left.OutputBytes != right.OutputBytes)
      {
        return left.OutputBytes > right.OutputBytes;
      }

      if (left.Channel != right.Channel)
        return left.Channel < right.Channel;

      return left.BackendType < right.BackendType;
    }
  );

  uint64_t totalRecords = 0;
  uint64_t totalBytes = 0;
  for (const BackendSnapshot& backend : backends)
  {
    totalRecords += backend.Records;
    totalBytes += backend.OutputBytes;
  }

  double duration = GetDurationSeconds(active);
  std::string response;
  AppendOutputHeader(
    response
    , active
    , duration
    , sort
    , totalRecords
    , totalBytes
    , backendType
  );

  if (backends.empty())
  {
    response += "No backend output collected.\n";
    return response;
  }

  size_t count = std::min(limit, backends.size());
  for (size_t i = 0; i < count; ++i)
  {
    const BackendSnapshot& backend = backends[i];
    uint64_t shareValue = sort == LogStatisticsSort::RECORDS
      ? backend.Records
      : backend.OutputBytes;
    uint64_t shareTotal = sort == LogStatisticsSort::RECORDS
      ? totalRecords
      : totalBytes;

    char line[640];
    snprintf(
      line
      , sizeof(line)
      , "%zu. share=%.2f%% records=%llu records/s=%.2f output-bytes=%llu KiB/s=%.2f avg=%.1f max=%llu channel=%s backend=%s\n"
      , i + 1
      , GetShare(shareValue, shareTotal)
      , static_cast<unsigned long long>(backend.Records)
      , GetRate(backend.Records, duration)
      , static_cast<unsigned long long>(backend.OutputBytes)
      , GetRate(backend.OutputBytes, duration) / 1024.0
      , GetAverage(backend.OutputBytes, backend.Records)
      , static_cast<unsigned long long>(backend.MaxOutputBytes)
      , backend.Channel.c_str()
      , backend.BackendType.c_str()
    );
    response += line;
  }

  return response;
}

std::string LogStatisticsCollector::FormatFileBackends(
  bool active
  , LogFileStatisticsSort sort
  , size_t limit
) const
{
  std::vector<FileBackendSnapshot> backends = SnapshotFileBackends();

  auto sortValue = [sort](const FileBackendSnapshot& backend) -> uint64_t
  {
    switch (sort)
    {
      case LogFileStatisticsSort::WRITE_BATCHES:
        return backend.WriteBatches;

      case LogFileStatisticsSort::WRITE_ERRORS:
        return backend.FailedWriteOperations;

      case LogFileStatisticsSort::DROPPED_BYTES:
        return backend.QueueDroppedBytes;

      case LogFileStatisticsSort::WRITTEN_BYTES:
      default:
        return backend.WrittenBytes;
    }
  };

  std::sort(
    backends.begin()
    , backends.end()
    , [&sortValue](
      const FileBackendSnapshot& left
      , const FileBackendSnapshot& right
    )
    {
      uint64_t leftValue = sortValue(left);
      uint64_t rightValue = sortValue(right);
      if (leftValue != rightValue)
        return leftValue > rightValue;

      if (left.Channel != right.Channel)
        return left.Channel < right.Channel;

      return left.BackendId < right.BackendId;
    }
  );

  uint64_t totalSortValue = 0;
  for (const FileBackendSnapshot& backend : backends)
    totalSortValue += sortValue(backend);

  double duration = GetDurationSeconds(active);
  std::string response;
  char header[512];
  snprintf(
    header
    , sizeof(header)
    , "Async FileBackend statistics: %s\nDuration: %.3f s\nSort: %s\nBackends: %zu\n"
    , active ? "running" : "stopped"
    , duration
    , GetFileSortName(sort)
    , backends.size()
  );
  response += header;

  if (backends.empty())
  {
    response += "No asynchronous FileBackend activity collected.\n";
    return response;
  }

  size_t count = std::min(limit, backends.size());
  for (size_t i = 0; i < count; ++i)
  {
    const FileBackendSnapshot& backend = backends[i];
    uint64_t value = sortValue(backend);
    char line[1024];
    snprintf(
      line
      , sizeof(line)
      , "\n%zu. share=%.2f%% channel=%s backend=%s async=%s\n"
        "   accepted: records=%llu bytes=%llu records/s=%.2f KiB/s=%.2f\n"
        "   worker: batches=%llu operations=%llu buffers=%llu input-bytes=%llu avg-buffers=%.1f avg-bytes=%.1f max-buffers=%llu max-bytes=%llu\n"
        "   written: buffers=%llu bytes=%llu KiB/s=%.2f\n"
        "   failures: batches=%llu operations=%llu buffers=%llu input-bytes=%llu\n"
        "   queue-dropped: records=%llu bytes=%llu\n"
      , i + 1
      , GetShare(value, totalSortValue)
      , backend.Channel.c_str()
      , backend.BackendType.c_str()
      , backend.Async ? "true" : "false"
      , static_cast<unsigned long long>(backend.AcceptedRecords)
      , static_cast<unsigned long long>(backend.AcceptedBytes)
      , GetRate(backend.AcceptedRecords, duration)
      , GetRate(backend.AcceptedBytes, duration) / 1024.0
      , static_cast<unsigned long long>(backend.WriteBatches)
      , static_cast<unsigned long long>(backend.WriteOperations)
      , static_cast<unsigned long long>(backend.BatchBuffers)
      , static_cast<unsigned long long>(backend.BatchBytes)
      , GetAverage(backend.BatchBuffers, backend.WriteBatches)
      , GetAverage(backend.BatchBytes, backend.WriteBatches)
      , static_cast<unsigned long long>(backend.MaxBatchBuffers)
      , static_cast<unsigned long long>(backend.MaxBatchBytes)
      , static_cast<unsigned long long>(backend.WrittenBuffers)
      , static_cast<unsigned long long>(backend.WrittenBytes)
      , GetRate(backend.WrittenBytes, duration) / 1024.0
      , static_cast<unsigned long long>(backend.FailedBatches)
      , static_cast<unsigned long long>(backend.FailedWriteOperations)
      , static_cast<unsigned long long>(backend.FailedBuffers)
      , static_cast<unsigned long long>(backend.FailedBytes)
      , static_cast<unsigned long long>(backend.QueueDroppedRecords)
      , static_cast<unsigned long long>(backend.QueueDroppedBytes)
    );
    response += line;

    if (!backend.Details.empty())
    {
      response += "   details: ";
      response += backend.Details;
      response += "\n";
    }
  }

  return response;
}

void Logger::RecordLogBackendOutput(
  Context& context
  , Channel* channel
  , Backend* backend
  , size_t outputBytes
)
{
  LogStatisticsCollector* statistics = ActiveLogStatistics.load(
    std::memory_order_relaxed
  );

  if (statistics == nullptr)
    return;

  std::atomic_thread_fence(std::memory_order_acquire);
  statistics->Enter();

  if (ActiveLogStatistics.load(std::memory_order_acquire) == statistics)
  {
    LogSiteStatistics* site = LoadCachedSite(
      context
      , statistics->GetGeneration()
    );

    if (site != nullptr)
      statistics->RecordBackend(site, channel, backend, outputBytes);
  }

  statistics->Leave();
}


void Logger::RecordFileBackendQueueDrop(
  Channel* channel
  , Backend* backend
  , size_t outputBytes
)
{
  LogStatisticsCollector* statistics = ActiveLogStatistics.load(
    std::memory_order_relaxed
  );

  if (statistics == nullptr)
    return;

  std::atomic_thread_fence(std::memory_order_acquire);
  statistics->Enter();

  if (ActiveLogStatistics.load(std::memory_order_acquire) == statistics)
  {
    statistics->RecordFileBackendQueueDrop(
      channel
      , backend
      , outputBytes
    );
  }

  statistics->Leave();
}

void Logger::RecordFileBackendWrite(
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
)
{
  LogStatisticsCollector* statistics = ActiveLogStatistics.load(
    std::memory_order_relaxed
  );

  if (statistics == nullptr)
    return;

  std::atomic_thread_fence(std::memory_order_acquire);
  statistics->Enter();

  if (ActiveLogStatistics.load(std::memory_order_acquire) == statistics)
  {
    statistics->RecordFileBackendWrite(
      channel
      , backend
      , batchBuffers
      , batchBytes
      , writeOperations
      , writtenBuffers
      , writtenBytes
      , failedWriteOperations
      , failedBuffers
      , failedBytes
    );
  }

  statistics->Leave();
}

void Logger::StartLogStatistics()
{
  std::lock_guard guard(LogStatisticsControlLock);

  LogStatisticsCollector* previous = ActiveLogStatistics.exchange(
    nullptr
    , std::memory_order_acq_rel
  );

  if (previous != nullptr)
    previous->WaitForIdle();

  if (LogStatistics == nullptr)
    LogStatistics = std::make_unique<LogStatisticsCollector>();

  LogStatistics->Start();
  ActiveLogStatistics.store(LogStatistics.get(), std::memory_order_release);
}

void Logger::StopLogStatistics()
{
  std::lock_guard guard(LogStatisticsControlLock);

  LogStatisticsCollector* statistics = ActiveLogStatistics.exchange(
    nullptr
    , std::memory_order_acq_rel
  );

  if (statistics == nullptr)
    return;

  statistics->WaitForIdle();
  statistics->Stop();
}

void Logger::ResetLogStatistics()
{
  std::lock_guard guard(LogStatisticsControlLock);

  LogStatisticsCollector* statistics = ActiveLogStatistics.exchange(
    nullptr
    , std::memory_order_acq_rel
  );

  if (statistics != nullptr)
    statistics->WaitForIdle();

  if (LogStatistics != nullptr)
    LogStatistics->Reset();

  if (statistics != nullptr)
    ActiveLogStatistics.store(statistics, std::memory_order_release);
}

bool Logger::IsLogStatisticsActive() const
{
  return ActiveLogStatistics.load(std::memory_order_relaxed) != nullptr;
}

std::string Logger::DumpLogStatisticsStatus()
{
  std::lock_guard guard(LogStatisticsControlLock);

  if (LogStatistics == nullptr)
  {
    return
      "Log statistics: stopped\n"
      "Session: not started\n"
      "Duration: 0.000 s\n"
      "Sites: 0\n"
      "Records: 0\n"
      "Message bytes: 0\n"
      "Backend records: 0\n"
      "Backend output bytes: 0\n"
      "Async file backend rows: 0\n"
      "File write batches: 0\n"
      "File written bytes: 0\n"
      "File failed batches: 0\n"
      "File queue dropped bytes: 0\n";
  }

  return LogStatistics->FormatStatus(IsLogStatisticsActive());
}

std::string Logger::DumpLogStatisticsTop(
  LogStatisticsSort sort
  , size_t limit
)
{
  std::lock_guard guard(LogStatisticsControlLock);

  if (LogStatistics == nullptr)
  {
    return
      "Log statistics: stopped\n"
      "Duration: 0.000 s\n"
      "No log records collected.\n";
  }

  return LogStatistics->FormatTop(
    IsLogStatisticsActive()
    , sort
    , limit
  );
}

std::string Logger::DumpLogStatisticsChannels(
  LogStatisticsSort sort
  , size_t limit
)
{
  std::lock_guard guard(LogStatisticsControlLock);

  if (LogStatistics == nullptr)
  {
    return
      "Log statistics: stopped\n"
      "Duration: 0.000 s\n"
      "No log records collected.\n";
  }

  return LogStatistics->FormatChannels(
    IsLogStatisticsActive()
    , sort
    , limit
  );
}

std::string Logger::DumpLogStatisticsOutputs(
  LogStatisticsSort sort
  , size_t limit
  , const std::string& backendType
)
{
  std::lock_guard guard(LogStatisticsControlLock);

  if (LogStatistics == nullptr)
  {
    return
      "Log backend statistics: stopped\n"
      "Duration: 0.000 s\n"
      "No backend output collected.\n";
  }

  return LogStatistics->FormatOutputs(
    IsLogStatisticsActive()
    , sort
    , limit
    , backendType
  );
}

std::string Logger::DumpLogStatisticsBackends(
  LogStatisticsSort sort
  , size_t limit
  , const std::string& backendType
)
{
  std::lock_guard guard(LogStatisticsControlLock);

  if (LogStatistics == nullptr)
  {
    return
      "Log backend statistics: stopped\n"
      "Duration: 0.000 s\n"
      "No backend output collected.\n";
  }

  return LogStatistics->FormatBackends(
    IsLogStatisticsActive()
    , sort
    , limit
    , backendType
  );
}

std::string Logger::DumpLogStatisticsFileBackends(
  LogFileStatisticsSort sort
  , size_t limit
)
{
  std::lock_guard guard(LogStatisticsControlLock);

  if (LogStatistics == nullptr)
  {
    return
      "Async FileBackend statistics: stopped\n"
      "Duration: 0.000 s\n"
      "No asynchronous FileBackend activity collected.\n";
  }

  return LogStatistics->FormatFileBackends(
    IsLogStatisticsActive()
    , sort
    , limit
  );
}
