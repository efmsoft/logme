#include <cassert>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <regex>
#include <sstream>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include <Logme/Backend/FileBackend.h>
#include <Logme/Channel.h>
#include <Logme/Context.h>

#include <Logme/File/exe_path.h>

#include <Logme/File/FileManager.h>
#include <Logme/File/FileManagerFactory.h>
#include <Logme/File/RetentionCleaner.h>

#include "../File/FileArchivePolicy.h"
#include "../File/FileTimeRotationPolicy.h"
#include <Logme/Logger.h>
#include <Logme/Logme.h>
#include <Logme/Template.h>
#include <Logme/Time/datetime.h> 
#include <Logme/Types.h>


using namespace std::chrono_literals;

#ifndef _WIN32
#include <sys/file.h>
#include <sys/uio.h>
#include <unistd.h>

#define _lseek lseek
#define _read read
#define _write write

#else
#include <io.h>
#define ftruncate _chsize

#endif

using namespace Logme;

namespace
{
  std::atomic<std::uint64_t> GlobalDisplayCalls(0);
  std::atomic<std::uint64_t> GlobalAppendCalls(0);
  std::atomic<std::uint64_t> GlobalInputBytes(0);
  std::atomic<std::uint64_t> GlobalOutputBytes(0);
  std::atomic<std::uint64_t> GlobalFlushRequests(0);
  std::atomic<std::uint64_t> GlobalImmediateFlushRequests(0);
  std::atomic<std::uint64_t> GlobalScheduledFlushRequests(0);
  std::atomic<std::uint64_t> GlobalFlushAccepted(0);
  std::atomic<std::uint64_t> GlobalImmediateFlushAccepted(0);
  std::atomic<std::uint64_t> GlobalScheduledFlushAccepted(0);
  std::atomic<std::uint64_t> GlobalFlushRejected(0);
  std::atomic<std::uint64_t> GlobalImmediateFlushRejected(0);
  std::atomic<std::uint64_t> GlobalScheduledFlushRejected(0);
  std::atomic<std::uint64_t> GlobalFlushWaitCalls(0);
  std::atomic<std::uint64_t> GlobalWorkerRuns(0);
  std::atomic<std::uint64_t> GlobalWorkerImmediateRuns(0);
  std::atomic<std::uint64_t> GlobalWorkerTimedRuns(0);
  std::atomic<std::uint64_t> GlobalWorkerShutdownRuns(0);
  std::atomic<std::uint64_t> GlobalWriteReadyCalls(0);
  std::atomic<std::uint64_t> GlobalWriteBatches(0);
  std::atomic<std::uint64_t> GlobalWriteBatchBytes(0);
  std::atomic<std::uint64_t> GlobalWriteBatchMaxBytes(0);
  std::atomic<std::uint64_t> GlobalWrittenBuffers(0);
  std::atomic<std::uint64_t> GlobalWrittenBytes(0);
  std::atomic<std::uint64_t> GlobalWriteErrors(0);

#if FILE_ENABLE_WRITE_READY_COUNTERS
  std::atomic<std::uint64_t> GlobalWriteReadyEmptyCalls(0);
  std::atomic<std::uint64_t> GlobalWriteReadyRawCalls(0);
  std::atomic<std::uint64_t> GlobalWriteReadyRawBytes(0);
  std::atomic<std::uint64_t> GlobalWriteReadyRawMaxBytes(0);
  std::atomic<std::uint64_t> GlobalWriteReadyMaxBuffers(0);
  std::atomic<std::uint64_t> GlobalWriteReadyMaxBytes(0);
  std::atomic<std::uint64_t> GlobalWorkerWriteLoopIterations(0);
  std::atomic<std::uint64_t> GlobalWorkerWriteLoopMaxIterations(0);
  std::atomic<std::uint64_t> GlobalWorkerPublishCurrentCalls(0);
  std::atomic<std::uint64_t> GlobalWorkerPublishCurrentSuccess(0);
  std::atomic<std::uint64_t> GlobalWorkerBreaks(0);

  void UpdateMaxCounter(
    std::atomic<std::uint64_t>& counter
    , std::uint64_t value
  )
  {
    std::uint64_t oldMax = counter.load(std::memory_order_relaxed);
    while (
      oldMax < value
      && !counter.compare_exchange_weak(
        oldMax
        , value
        , std::memory_order_relaxed
        , std::memory_order_relaxed
      )
    )
    {
    }
  }
#endif

#if FILE_ENABLE_FLUSH_SOURCE_COUNTERS
  std::atomic<std::uint64_t> GlobalPublishFromFlushCalls(0);
  std::atomic<std::uint64_t> GlobalPublishFromFlushSuccess(0);
  std::atomic<std::uint64_t> GlobalPublishFromWorkerImmediateCalls(0);
  std::atomic<std::uint64_t> GlobalPublishFromWorkerImmediateSuccess(0);
  std::atomic<std::uint64_t> GlobalPublishFromWorkerShutdownCalls(0);
  std::atomic<std::uint64_t> GlobalPublishFromWorkerShutdownSuccess(0);
  std::atomic<std::uint64_t> GlobalPublishFromOnShutdownCalls(0);
  std::atomic<std::uint64_t> GlobalPublishFromOnShutdownSuccess(0);
  std::atomic<std::uint64_t> GlobalRequestRightNowFromFlush(0);
  std::atomic<std::uint64_t> GlobalRequestRightNowFromFreeze(0);
  std::atomic<std::uint64_t> GlobalRequestRightNowFromPressureBuffers(0);
  std::atomic<std::uint64_t> GlobalRequestRightNowFromPressureBytes(0);
  std::atomic<std::uint64_t> GlobalRequestRightNowFromPressureBoth(0);
  std::atomic<std::uint64_t> GlobalRequestScheduledFromFirstData(0);
  std::atomic<std::uint64_t> GlobalPressureByBuffers(0);
  std::atomic<std::uint64_t> GlobalPressureByBytes(0);
  std::atomic<std::uint64_t> GlobalPressureByBoth(0);
  std::atomic<std::uint64_t> GlobalPublishCurrentQueuedBytes(0);
  std::atomic<std::uint64_t> GlobalPublishCurrentMaxQueuedBytes(0);
  std::atomic<std::uint64_t> GlobalPublishCurrentAgeMs(0);
  std::atomic<std::uint64_t> GlobalPublishCurrentMaxAgeMs(0);
#endif

  std::atomic<std::uint64_t> GlobalCreateLogCalls(0);
  std::atomic<std::uint64_t> GlobalCreateLogFailures(0);
  std::atomic<std::uint64_t> GlobalChangePartCalls(0);
  std::atomic<std::uint64_t> GlobalChangePartFailures(0);
  std::atomic<std::uint64_t> GlobalSizeLimitCompletionCalls(0);
  std::atomic<std::uint64_t> GlobalTimeLimitCompletionCalls(0);
  std::atomic<std::uint64_t> GlobalArchivedFiles(0);
  std::atomic<std::uint64_t> GlobalCompressionSubmitCalls(0);
  std::atomic<std::uint64_t> GlobalRetentionRuns(0);
  std::atomic<std::uint64_t> GlobalShutdownCalls(0);
}

size_t FileBackend::MaxSizeDefault = FileBackend::MAX_SIZE_DEFAULT;
size_t FileBackend::QueueSizeLimitDefault = FileBackend::QUEUE_SIZE_LIMIT;
size_t FileBackend::QueueBufferLimitDefault = FileBackend::MAX_TOTAL_BUFFERS;
uint64_t FileBackend::FlushAfterDefault = FileBackend::FLUSH_AFTER_DEFAULT;
size_t FileBackend::DataBufferSizeDefault = FileBackend::QUEUE_BUFFER_SIZE;
std::atomic<size_t> FileBackend::DataBufferCacheLimit(16);
std::atomic<size_t> FileBackend::DataBufferCacheMaxLimit(
  FileBackend::DATA_BUFFER_CACHE_MAX_LIMIT_DEFAULT
);
std::atomic<uint64_t> FileBackend::DataBufferCacheRetainOverLimitMs(
  FileBackend::DATA_BUFFER_CACHE_RETAIN_OVER_LIMIT_MS_DEFAULT
);

FileBackend::FileBackend(ChannelPtr owner)
  : MemoryTrackedBackend(owner, TYPE_ID)
  , Append(true)
  , MaxSize(MaxSizeDefault)
  , OnSizeLimit(SIZE_LIMIT_TRUNCATE)
  , CurrentSize(0)
  , QueueSizeLimit(QueueSizeLimitDefault)
  , FlushAfter(FlushAfterDefault)
  , ArchivePolicy(new FileArchivePolicy())
  , Registered(false)
  , ShutdownFlag(false)
  , ShutdownCalled(owner == nullptr)
  , FlushTime(0)
  , ActiveNext(nullptr)
  , ActivePrev(nullptr)
  , ActiveLinked(false)
  , Queue(
    Owner.get()
    , [owner]
    {
      BufferQueue::Options o;
      o.BufferSize = DataBufferSizeDefault;
      o.MaxTotalBuffers = QueueBufferLimitDefault;
      o.CacheContext = owner.get();
      o.TakeCachedBuffer = &FileBackend::TakeCachedDataBuffer;
      o.ReturnCachedBuffer = &FileBackend::ReturnCachedDataBuffer;
      o.CountDataBufferAllocation = &FileBackend::CountDataBufferAllocation;
      return o;
    }()
    , GetMemoryUsageTracker()
  )
  , QueuedBytes(0)
  , TimeRotationPolicy(new FileTimeRotationPolicy())
  , MaxParts(2)
  , RetentionMaxAge(0)
  , RetentionMaxTotalSize(0)
  , RetentionCleanOnStart(true)
  , GzipCompression(false)
{
  SetAsync(true);
  NonceGenInit(&Nonce);
}

FileBackend::~FileBackend()
{
  assert(ShutdownFlag.load(std::memory_order_relaxed) || Registered.load(std::memory_order_relaxed) == false);

  if (ShutdownCalled.load(std::memory_order_relaxed) == false)
    WaitForShutdown();

  CloseLog();
}

void FileBackend::SetMaxSizeDefault(size_t size)
{
  MaxSizeDefault = size;
}

size_t FileBackend::GetMaxSizeDefault()
{
  return MaxSizeDefault;
}

void FileBackend::SetFlushAfterDefault(uint64_t ms)
{
  FlushAfterDefault = ms;
}

uint64_t FileBackend::GetFlushAfterDefault()
{
  return FlushAfterDefault;
}

size_t FileBackend::GetDataBufferSizeDefault()
{
  return DataBufferSizeDefault;
}

void FileBackend::SetDataBufferSizeDefault(size_t size)
{
  if (size == 0)
    size = QUEUE_BUFFER_SIZE;

  DataBufferSizeDefault = size;
}

size_t FileBackend::GetDataBufferCacheLimit()
{
  return DataBufferCacheLimit.load(std::memory_order_relaxed);
}

void FileBackend::SetDataBufferCacheLimit(size_t count)
{
  DataBufferCacheLimit.store(count, std::memory_order_relaxed);

  if (Logme::Instance)
    Logme::Instance->GetFileManagerFactory().TrimDataBufferCache(count);
}

size_t FileBackend::GetDataBufferCacheMaxLimit()
{
  return DataBufferCacheMaxLimit.load(std::memory_order_relaxed);
}

void FileBackend::SetDataBufferCacheMaxLimit(size_t count)
{
  DataBufferCacheMaxLimit.store(count, std::memory_order_relaxed);

  if (Logme::Instance)
    Logme::Instance->GetFileManagerFactory().TrimDataBufferCacheMaxLimit(count);
}

uint64_t FileBackend::GetDataBufferCacheRetainOverLimitMs()
{
  return DataBufferCacheRetainOverLimitMs.load(std::memory_order_relaxed);
}

void FileBackend::SetDataBufferCacheRetainOverLimitMs(uint64_t ms)
{
  DataBufferCacheRetainOverLimitMs.store(ms, std::memory_order_relaxed);
}

void FileBackend::ClearDataBufferCache()
{
  if (Logme::Instance)
    Logme::Instance->GetFileManagerFactory().ClearDataBufferCache();
}

size_t FileBackend::GetQueueSizeLimitDefault()
{
  return QueueSizeLimitDefault;
}

void FileBackend::SetQueueBufferLimitDefault(size_t count)
{
  QueueBufferLimitDefault = count;
}

size_t FileBackend::GetQueueBufferLimitDefault()
{
  return QueueBufferLimitDefault;
}

FileBackendCounters FileBackend::GetCounters()
{
  FileBackendCounters out;
  out.DisplayCalls = GlobalDisplayCalls.load(std::memory_order_relaxed);
  out.AppendCalls = GlobalAppendCalls.load(std::memory_order_relaxed);
  out.InputBytes = GlobalInputBytes.load(std::memory_order_relaxed);
  out.OutputBytes = GlobalOutputBytes.load(std::memory_order_relaxed);
  out.FlushRequests = GlobalFlushRequests.load(std::memory_order_relaxed);
  out.ImmediateFlushRequests = GlobalImmediateFlushRequests.load(std::memory_order_relaxed);
  out.ScheduledFlushRequests = GlobalScheduledFlushRequests.load(std::memory_order_relaxed);
  out.FlushAccepted = GlobalFlushAccepted.load(std::memory_order_relaxed);
  out.ImmediateFlushAccepted = GlobalImmediateFlushAccepted.load(std::memory_order_relaxed);
  out.ScheduledFlushAccepted = GlobalScheduledFlushAccepted.load(std::memory_order_relaxed);
  out.FlushRejected = GlobalFlushRejected.load(std::memory_order_relaxed);
  out.ImmediateFlushRejected = GlobalImmediateFlushRejected.load(std::memory_order_relaxed);
  out.ScheduledFlushRejected = GlobalScheduledFlushRejected.load(std::memory_order_relaxed);
  out.FlushWaitCalls = GlobalFlushWaitCalls.load(std::memory_order_relaxed);
  out.WorkerRuns = GlobalWorkerRuns.load(std::memory_order_relaxed);
  out.WorkerImmediateRuns = GlobalWorkerImmediateRuns.load(std::memory_order_relaxed);
  out.WorkerTimedRuns = GlobalWorkerTimedRuns.load(std::memory_order_relaxed);
  out.WorkerShutdownRuns = GlobalWorkerShutdownRuns.load(std::memory_order_relaxed);
  out.WriteReadyCalls = GlobalWriteReadyCalls.load(std::memory_order_relaxed);
  out.WriteBatches = GlobalWriteBatches.load(std::memory_order_relaxed);
  out.WriteBatchBytes = GlobalWriteBatchBytes.load(std::memory_order_relaxed);
  out.WriteBatchMaxBytes = GlobalWriteBatchMaxBytes.load(std::memory_order_relaxed);
  out.WrittenBuffers = GlobalWrittenBuffers.load(std::memory_order_relaxed);
  out.WrittenBytes = GlobalWrittenBytes.load(std::memory_order_relaxed);
  out.WriteErrors = GlobalWriteErrors.load(std::memory_order_relaxed);
#if FILE_ENABLE_WRITE_READY_COUNTERS
  out.WriteReadyEmptyCalls =
    GlobalWriteReadyEmptyCalls.load(std::memory_order_relaxed);
  out.WriteReadyRawCalls =
    GlobalWriteReadyRawCalls.load(std::memory_order_relaxed);
  out.WriteReadyRawBytes =
    GlobalWriteReadyRawBytes.load(std::memory_order_relaxed);
  out.WriteReadyRawMaxBytes =
    GlobalWriteReadyRawMaxBytes.load(std::memory_order_relaxed);
  out.WriteReadyMaxBuffers =
    GlobalWriteReadyMaxBuffers.load(std::memory_order_relaxed);
  out.WriteReadyMaxBytes =
    GlobalWriteReadyMaxBytes.load(std::memory_order_relaxed);
  out.WorkerWriteLoopIterations =
    GlobalWorkerWriteLoopIterations.load(std::memory_order_relaxed);
  out.WorkerWriteLoopMaxIterations =
    GlobalWorkerWriteLoopMaxIterations.load(std::memory_order_relaxed);
  out.WorkerPublishCurrentCalls =
    GlobalWorkerPublishCurrentCalls.load(std::memory_order_relaxed);
  out.WorkerPublishCurrentSuccess =
    GlobalWorkerPublishCurrentSuccess.load(std::memory_order_relaxed);
  out.WorkerBreaks =
    GlobalWorkerBreaks.load(std::memory_order_relaxed);
#endif
#if FILE_ENABLE_FLUSH_SOURCE_COUNTERS
  out.PublishFromFlushCalls =
    GlobalPublishFromFlushCalls.load(std::memory_order_relaxed);
  out.PublishFromFlushSuccess =
    GlobalPublishFromFlushSuccess.load(std::memory_order_relaxed);
  out.PublishFromWorkerImmediateCalls =
    GlobalPublishFromWorkerImmediateCalls.load(std::memory_order_relaxed);
  out.PublishFromWorkerImmediateSuccess =
    GlobalPublishFromWorkerImmediateSuccess.load(std::memory_order_relaxed);
  out.PublishFromWorkerShutdownCalls =
    GlobalPublishFromWorkerShutdownCalls.load(std::memory_order_relaxed);
  out.PublishFromWorkerShutdownSuccess =
    GlobalPublishFromWorkerShutdownSuccess.load(std::memory_order_relaxed);
  out.PublishFromOnShutdownCalls =
    GlobalPublishFromOnShutdownCalls.load(std::memory_order_relaxed);
  out.PublishFromOnShutdownSuccess =
    GlobalPublishFromOnShutdownSuccess.load(std::memory_order_relaxed);
  out.RequestRightNowFromFlush =
    GlobalRequestRightNowFromFlush.load(std::memory_order_relaxed);
  out.RequestRightNowFromFreeze =
    GlobalRequestRightNowFromFreeze.load(std::memory_order_relaxed);
  out.RequestRightNowFromPressureBuffers =
    GlobalRequestRightNowFromPressureBuffers.load(std::memory_order_relaxed);
  out.RequestRightNowFromPressureBytes =
    GlobalRequestRightNowFromPressureBytes.load(std::memory_order_relaxed);
  out.RequestRightNowFromPressureBoth =
    GlobalRequestRightNowFromPressureBoth.load(std::memory_order_relaxed);
  out.RequestScheduledFromFirstData =
    GlobalRequestScheduledFromFirstData.load(std::memory_order_relaxed);
  out.PressureByBuffers =
    GlobalPressureByBuffers.load(std::memory_order_relaxed);
  out.PressureByBytes =
    GlobalPressureByBytes.load(std::memory_order_relaxed);
  out.PressureByBoth =
    GlobalPressureByBoth.load(std::memory_order_relaxed);
  out.PublishCurrentQueuedBytes =
    GlobalPublishCurrentQueuedBytes.load(std::memory_order_relaxed);
  out.PublishCurrentMaxQueuedBytes =
    GlobalPublishCurrentMaxQueuedBytes.load(std::memory_order_relaxed);
  out.PublishCurrentAgeMs =
    GlobalPublishCurrentAgeMs.load(std::memory_order_relaxed);
  out.PublishCurrentMaxAgeMs =
    GlobalPublishCurrentMaxAgeMs.load(std::memory_order_relaxed);
#endif

  out.CreateLogCalls = GlobalCreateLogCalls.load(std::memory_order_relaxed);
  out.CreateLogFailures = GlobalCreateLogFailures.load(std::memory_order_relaxed);
  out.ChangePartCalls = GlobalChangePartCalls.load(std::memory_order_relaxed);
  out.ChangePartFailures = GlobalChangePartFailures.load(std::memory_order_relaxed);
  out.SizeLimitCompletionCalls = GlobalSizeLimitCompletionCalls.load(std::memory_order_relaxed);
  out.TimeLimitCompletionCalls = GlobalTimeLimitCompletionCalls.load(std::memory_order_relaxed);
  out.ArchivedFiles = GlobalArchivedFiles.load(std::memory_order_relaxed);
  out.CompressionSubmitCalls = GlobalCompressionSubmitCalls.load(std::memory_order_relaxed);
  out.RetentionRuns = GlobalRetentionRuns.load(std::memory_order_relaxed);
  out.ShutdownCalls = GlobalShutdownCalls.load(std::memory_order_relaxed);
  out.Queue = BufferQueue::GetGlobalCounters();
  return out;
}

void FileBackend::SetQueueSizeLimitDefault(size_t size)
{
  if (size < 2ULL * QUEUE_BUFFER_SIZE)
    size = 2ULL * QUEUE_BUFFER_SIZE;

  QueueSizeLimitDefault = size;
}


DataBufferPtr FileBackend::TakeCachedDataBuffer(
  void* context
  , MemoryUsageTracker* memoryTracker
  , std::size_t capacity
)
{
  const size_t cacheLimit = GetDataBufferCacheLimit();

  if (cacheLimit == 0 || !context)
    return nullptr;

  Channel* channel = static_cast<Channel*>(context);
  Logger* logger = channel->GetOwner();

  if (!logger)
    return nullptr;

  return logger->GetFileManagerFactory().TakeDataBuffer(
    memoryTracker
    , capacity
    , cacheLimit
    , GetDataBufferCacheMaxLimit()
    , GetDataBufferCacheRetainOverLimitMs()
  );
}

bool FileBackend::ReturnCachedDataBuffer(void* context, DataBufferPtr buffer)
{
  const size_t cacheLimit = GetDataBufferCacheLimit();

  if (cacheLimit == 0 || !context || !buffer)
    return false;

  Channel* channel = static_cast<Channel*>(context);
  Logger* logger = channel->GetOwner();

  if (!logger)
    return false;

  return logger->GetFileManagerFactory().ReturnDataBuffer(
    std::move(buffer)
    , cacheLimit
    , GetDataBufferCacheMaxLimit()
    , GetDataBufferCacheRetainOverLimitMs()
  );
}

void FileBackend::CountDataBufferAllocation(void* context)
{
  (void)context;
  FileManager::CountDataBufferAllocation();
}

std::string FileBackend::FormatDetails()
{
  std::ostringstream os;
  os << NameTemplate;
  os << (Append ? " APPEND" : " OVERWRITE");
  os << " MaxSize=" << MaxSize;
  os << " OnSizeLimit=" << (OnSizeLimit == SIZE_LIMIT_ROTATE ? "ROTATE" : "TRUNCATE");
  if (ArchivePolicy->IsEnabled())
    os << " Archive=" << ArchivePolicy->GetTemplate();
  os << " Rotation=" << TimeRotationPolicy->GetModeName();
  os << " MaxParts=" << MaxParts;
  if (RetentionMaxAge != 0)
    os << " RetentionMaxAge=" << RetentionMaxAge;
  if (RetentionMaxTotalSize != 0)
    os << " RetentionMaxTotalSize=" << RetentionMaxTotalSize;
  os << " GzipCompression=" << (GzipCompression ? "YES" : "NO");
  os << " Async=" << (GetAsync() ? "YES" : "NO");

  size_t memoryUsage = GetMemoryUsage();
  if (memoryUsage != 0)
    os << " Memory=" << memoryUsage;

  return os.str();
}

BackendConfigPtr FileBackend::CreateConfig()
{
  return std::make_shared<FileBackendConfig>();
}

bool FileBackend::IsAsyncSupported() const
{
  return true;
}

void FileBackend::Flush()
{
  if (!GetAsync())
  {
    GetActiveIo().Flush();
    return;
  }

  FILE_CNT(GlobalFlushWaitCalls.fetch_add(1, std::memory_order_relaxed));
  for (;;)
  {
    bool needSignal = false;
    if (!PublishCurrentCounted(needSignal, PublishCurrentSource::FLUSH))
      break;
  }

  RequestFlush(RIGHT_NOW, FlushRequestSource::FLUSH);

  std::unique_lock locker(BufferLock);
  Done.wait(locker, [this]() { return QueuedBytes.load(std::memory_order_relaxed) == 0; });
}

void FileBackend::Freeze()
{
  ShutdownFlag.store(true, std::memory_order_relaxed);
  Backend::Freeze();

  if (GetAsync())
    RequestFlush(RIGHT_NOW, FlushRequestSource::FREEZE);
}

bool FileBackend::IsIdle() const
{
  return QueuedBytes.load(std::memory_order_relaxed) == 0 && (ShutdownFlag.load(std::memory_order_relaxed) || Registered.load(std::memory_order_relaxed) == false);
}

uint64_t FileBackend::GetFlushTime() const
{
  return FlushTime.load(std::memory_order_relaxed);
}

bool FileBackend::ApplyConfig(BackendConfigPtr c)
{
  if (c == nullptr || c->Type != TYPE_ID)
    return false;

  FileBackendConfig* p = (FileBackendConfig*)c.get();

  TimeRotationMode rotation = p->TimeRotation;
  if (rotation == TIME_ROTATION_NONE && p->DailyRotation)
    rotation = TIME_ROTATION_DAILY;

  SetAsync(p->Async);
  SetAppend(rotation != TIME_ROTATION_NONE ? true : p->Append);
  SetMaxSize(p->MaxSize);

  OnSizeLimit = p->OnSizeLimit;
  ArchivePolicy->Configure(
    p->ArchiveFilename
    , Owner->GetOwner()->GetHomeDirectory()
    , p->GzipCompression
  );
  TimeRotationPolicy->Configure(rotation);
  MaxParts = p->MaxParts;
  RetentionMaxAge = p->RetentionMaxAge;
  RetentionMaxTotalSize = p->RetentionMaxTotalSize;
  RetentionCleanOnStart = p->RetentionCleanOnStart;
  GzipCompression = p->GzipCompression;

  if (GzipCompression)
    Compression = Owner->GetOwner()->GetCompressionManagerFactory().RegisterUser();
  else
    Compression.reset();

  NameTemplate = p->Filename;
  ArchivePolicy->SetTime(TimeRotationPolicy->GetArchiveTime());
  return ChangePart(RetentionCleanOnStart);
}

void FileBackend::WaitForShutdown()
{
  while (ShutdownCalled.load(std::memory_order_relaxed) == false && Registered.load(std::memory_order_relaxed))
  {
    std::unique_lock locker(BufferLock);

    Shutdown.wait_for(
      locker
      , std::chrono::milliseconds(50)
      , [this]() {return ShutdownCalled.load(std::memory_order_relaxed); }
    );
  }
}

void FileBackend::SetMaxSize(size_t size)
{
  assert(size > 1024 || size == 0);
  MaxSize = size;
}

void FileBackend::SetQueueLimit(size_t size)
{
  assert(size > 1024);
  QueueSizeLimit = size;
}

void FileBackend::SetAppend(bool append)
{
  Append = append;
}

bool FileBackend::CreateLog(const char* v)
{
  FILE_CNT(GlobalCreateLogCalls.fetch_add(1, std::memory_order_relaxed));

  std::string nameTemplate = v;
  NameTemplate = nameTemplate;

  ProcessTemplateParam param;
  std::string name = ProcessTemplate(nameTemplate.c_str(), param);

  if (!IsAbsolutePath(name))
    name = Owner->GetOwner()->GetHomeDirectory() + name;

  CloseLog();
  Name.clear();

  Name = name;

  bool opened = GetActiveIo().OpenIgnoringPathNotFound(Append);
  if (!opened && GetActiveIo().IsPathNotFoundError())
  {
    auto dir = std::filesystem::path(Name).parent_path();
    if (!dir.empty())
    {
      std::error_code ec;
      std::filesystem::create_directories(dir, ec);

      if (ec)
      {
        LogmeE(
          CHINT
          , "FileBackend(%s): create_directories() failed: %s"
          , Name.c_str()
          , ec.message().c_str()
        );
        FILE_CNT(GlobalCreateLogFailures.fetch_add(1, std::memory_order_relaxed));
        return false;
      }

      opened = GetActiveIo().Open(Append);
    }
    else
    {
      LogmeE(
        CHINT
        , "FileBackend(%s): open failed: %s"
        , Name.c_str()
        , ERRNO_STR(GetActiveIo().GetError())
      );
    }
  }

  if (!opened)
  {
    FILE_CNT(GlobalCreateLogFailures.fetch_add(1, std::memory_order_relaxed));
    return false;
  }

  if (!Append)
  {
    CurrentSize = 0;
    return true;
  }

  auto rc = GetActiveIo().Seek(0, SEEK_END);
  if (rc < 0)
  {
    FILE_CNT(GlobalCreateLogFailures.fetch_add(1, std::memory_order_relaxed));
    return false;
  }

  CurrentSize = (size_t)rc;
  return true;
}

void FileBackend::CloseLog()
{
  FileIo::Close();
  if (BufferedIo)
    BufferedIo->Close();
}

bool FileBackend::TestFileInUse(const std::string& file) const
{
  if (!IsLogOpen())
    return false;

  return Name == file;
}

size_t FileBackend::GetSize()
{
  if (!IsLogOpen())
    return static_cast<size_t>(-1);

  return CurrentSize;
}

bool FileBackend::ChangePart(bool applyRetention)
{
  return CompleteCurrentFile(
    FILE_COMPLETION_TIME_LIMIT
    , applyRetention
    , TimeRotationPolicy->GetArchiveTime()
  );
}

void FileBackend::RegisterAsync()
{
  if (Registered.load(std::memory_order_relaxed) == false)
  {
    std::unique_lock locker(BufferLock);
    if (Owner && Registered.load(std::memory_order_relaxed) == false)
    {
      FileBackendPtr self = std::static_pointer_cast<FileBackend>(shared_from_this());
      GetFactory().Add(self);
      Registered.store(true, std::memory_order_relaxed);
    }
  }
}

void FileBackend::Display(Context& context)
{
  FILE_CNT(GlobalDisplayCalls.fetch_add(1, std::memory_order_relaxed));
  if (!IsLogOpen() || ShutdownFlag.load(std::memory_order_relaxed))
    return;

  if (GetAsync())
    RegisterAsync();

  std::time_t completedArchiveTime = 0;
  if (TimeRotationPolicy->ShouldRotate(completedArchiveTime))
  {
    if (!CompleteCurrentFile(
      FILE_COMPLETION_TIME_LIMIT
      , true
      , completedArchiveTime
    ))
    {
      return;
    }
  }

  int nc;
  const char* buffer = context.Apply(Owner, Owner->GetFlags(), nc);
  AppendStringInternal(buffer, nc);
}

bool FileBackend::ApplySizeLimit(size_t add)
{
  if (MaxSize == 0)
    return true;

  if (OnSizeLimit == SIZE_LIMIT_ROTATE)
  {
    if (CurrentSize + add <= MaxSize)
      return true;

    return CompleteCurrentFile(FILE_COMPLETION_SIZE_LIMIT);
  }

  Truncate();
  return true;
}

void FileBackend::Truncate()
{
  if (MaxSize == 0 || CurrentSize < MaxSize)
    return;

  GetActiveIo().TruncateToMaxSize(MaxSize);

  auto rc = GetActiveIo().Seek(0, SEEK_END);
  if (rc >= 0)
    CurrentSize = (size_t)rc;
}

std::string FileBackend::GetPathName(int index)
{
  if (index < 0 || index > 1)
    return "";

  ProcessTemplateParam param;
  std::string name = ProcessTemplate(Name.c_str(), param);
  std::string pathName = name;

  if (!IsAbsolutePath(pathName))
    pathName = Owner->GetOwner()->GetHomeDirectory() + name;

  return pathName;   
}

void FileBackend::AppendString(const char* text, size_t len)
{
  std::lock_guard guard(Owner->GetDataLock());

  if (!IsLogOpen() || ShutdownFlag.load(std::memory_order_relaxed))
    return;

  if (GetAsync())
    RegisterAsync();
  AppendStringInternal(text, len);
}

void FileBackend::AppendStringInternal(const char* text, size_t len)
{
  if (text)
  {
    if (len == (size_t)-1)
      len = strlen(text);

    AppendObfuscated(text, len);
  }
}

void FileBackend::AppendObfuscated(const char* text, size_t add)
{
  auto key = Owner->GetOwner()->GetObfuscationKey();
  if (key == nullptr)
  {
    AppendOutputData(text, add);
    return;
  }

  auto output = ObfCalcRecordSize(add);
  if (output < 4ULL * 1024)
  {
    size_t size = 0;
    uint8_t buffer[4ULL * 1024];
    if (ObfEncryptRecord(key, &Nonce, (const uint8_t*)text, add, buffer, sizeof(buffer), &size))
      AppendOutputData((const char*)buffer, size);
  }
  else
  {
    size_t size = 0;
    std::vector<uint8_t> buffer(output);
    if (ObfEncryptRecord(key, &Nonce, (const uint8_t*)text, add, buffer.data(), buffer.size(), &size))
      AppendOutputData((const char*)buffer.data(), size);
  }
}

void FileBackend::AppendOutputData(const char* text, size_t add)
{
  if (ShutdownFlag.load(std::memory_order_relaxed))
    return;

  if (add == 0)
    return;

  if (!GetAsync())
  {
    FILE_CNT(GlobalAppendCalls.fetch_add(1, std::memory_order_relaxed));
    FILE_CNT(GlobalInputBytes.fetch_add(add, std::memory_order_relaxed));

    std::lock_guard guard(IoLock);
    if (!ApplySizeLimit(add))
      return;

    int rc = GetActiveIo().WriteAll(text, add);
    if (rc < 0)
    {
      FILE_CNT(GlobalWriteErrors.fetch_add(1, std::memory_order_relaxed));
      return;
    }

    CurrentSize += (size_t)rc;
    FILE_CNT(GlobalWrittenBytes.fetch_add((size_t)rc, std::memory_order_relaxed));
    FILE_CNT(GlobalOutputBytes.fetch_add((size_t)rc, std::memory_order_relaxed));
    return;
  }

  bool needSignal = false;
  bool firstData = false;

  uint64_t flushTime = FlushTime.load(std::memory_order_relaxed);
  uint64_t firstWriteTime = GetTimeInMillisec64();

  if (flushTime != 0 && flushTime != RIGHT_NOW)
    firstWriteTime = flushTime > FlushAfter
      ? flushTime - FlushAfter
      : firstWriteTime;

  if (Queue.Append(text, add, firstWriteTime, needSignal, firstData) == false)
    return;

  (void)needSignal;

  FILE_CNT(GlobalAppendCalls.fetch_add(1, std::memory_order_relaxed));
  FILE_CNT(GlobalInputBytes.fetch_add(add, std::memory_order_relaxed));

  size_t queued = QueuedBytes.fetch_add(add, std::memory_order_relaxed) + add;
  size_t usedBuffers = Queue.GetUsedBuffers();
  flushTime = FlushTime.load(std::memory_order_relaxed);

  if (usedBuffers >= FLUSH_PRESSURE_BUFFERS || queued >= QueueSizeLimit)
  {
    const bool pressureByBuffers = usedBuffers >= FLUSH_PRESSURE_BUFFERS;
    const bool pressureByBytes = queued >= QueueSizeLimit;
    FlushRequestSource source = FlushRequestSource::PRESSURE_BUFFERS;

    if (pressureByBuffers && pressureByBytes)
    {
      source = FlushRequestSource::PRESSURE_BOTH;
      FILE_FSCNT(GlobalPressureByBoth.fetch_add(1, std::memory_order_relaxed));
    }
    else if (pressureByBytes)
    {
      source = FlushRequestSource::PRESSURE_BYTES;
      FILE_FSCNT(GlobalPressureByBytes.fetch_add(1, std::memory_order_relaxed));
    }
    else
    {
      FILE_FSCNT(GlobalPressureByBuffers.fetch_add(1, std::memory_order_relaxed));
    }

    if (firstData)
    {
      firstWriteTime = GetTimeInMillisec64();
      if (flushTime != 0 && flushTime != RIGHT_NOW)
        firstWriteTime = flushTime > FlushAfter ? flushTime - FlushAfter : firstWriteTime;

      Queue.SetCurrentFirstWriteTime(firstWriteTime);
    }

    if (flushTime != RIGHT_NOW)
      RequestFlush(RIGHT_NOW, source);

    return;
  }

  if (firstData)
  {
    if (flushTime == 0)
    {
      uint64_t now = GetTimeInMillisec64();
      Queue.SetCurrentFirstWriteTime(now);
      RequestFlush(now + FlushAfter, FlushRequestSource::FIRST_DATA);
    }
    else
    {
      firstWriteTime = GetTimeInMillisec64();
      if (flushTime != RIGHT_NOW)
        firstWriteTime = flushTime > FlushAfter ? flushTime - FlushAfter : firstWriteTime;

      Queue.SetCurrentFirstWriteTime(firstWriteTime);
    }
  }
}

void FileBackend::RequestFlush(
  uint64_t when
  , FlushRequestSource source
)
{
  const bool immediate = when == RIGHT_NOW;
  FILE_CNT(GlobalFlushRequests.fetch_add(1, std::memory_order_relaxed));
  if (immediate)
    FILE_CNT(GlobalImmediateFlushRequests.fetch_add(1, std::memory_order_relaxed));
  else
    FILE_CNT(GlobalScheduledFlushRequests.fetch_add(1, std::memory_order_relaxed));

#if FILE_ENABLE_FLUSH_SOURCE_COUNTERS
  if (immediate)
  {
    if (source == FlushRequestSource::FLUSH)
      GlobalRequestRightNowFromFlush.fetch_add(1, std::memory_order_relaxed);
    else if (source == FlushRequestSource::FREEZE)
      GlobalRequestRightNowFromFreeze.fetch_add(1, std::memory_order_relaxed);
    else if (source == FlushRequestSource::PRESSURE_BUFFERS)
      GlobalRequestRightNowFromPressureBuffers.fetch_add(1, std::memory_order_relaxed);
    else if (source == FlushRequestSource::PRESSURE_BYTES)
      GlobalRequestRightNowFromPressureBytes.fetch_add(1, std::memory_order_relaxed);
    else if (source == FlushRequestSource::PRESSURE_BOTH)
      GlobalRequestRightNowFromPressureBoth.fetch_add(1, std::memory_order_relaxed);
  }
  else if (source == FlushRequestSource::FIRST_DATA)
  {
    GlobalRequestScheduledFromFirstData.fetch_add(1, std::memory_order_relaxed);
  }
#endif

  uint64_t old = FlushTime.load(std::memory_order_relaxed);

  for (;;)
  {
    if (old != 0 && (old == RIGHT_NOW || old <= when))
    {
      FILE_CNT(GlobalFlushRejected.fetch_add(1, std::memory_order_relaxed));
      if (immediate)
        FILE_CNT(GlobalImmediateFlushRejected.fetch_add(1, std::memory_order_relaxed));
      else
        FILE_CNT(GlobalScheduledFlushRejected.fetch_add(1, std::memory_order_relaxed));
      return;
    }

    if (FlushTime.compare_exchange_weak(
      old
      , when
      , std::memory_order_relaxed
      , std::memory_order_relaxed
    ))
    {
      FILE_CNT(GlobalFlushAccepted.fetch_add(1, std::memory_order_relaxed));
      if (immediate)
        FILE_CNT(GlobalImmediateFlushAccepted.fetch_add(1, std::memory_order_relaxed));
      else
        FILE_CNT(GlobalScheduledFlushAccepted.fetch_add(1, std::memory_order_relaxed));

      if (Owner)
        GetFactory().Notify(this, when);

      return;
    }
  }
}

bool FileBackend::HasEvents() const
{
  return FlushTime.load(std::memory_order_relaxed) != 0;
}


FileIo& FileBackend::GetActiveIo()
{
  if (!GetAsync())
  {
    if (!BufferedIo)
      BufferedIo = std::make_unique<BufferedFileIo>(this);

    return *BufferedIo;
  }

  return *this;
}

const FileIo& FileBackend::GetActiveIo() const
{
  if (!GetAsync() && BufferedIo)
    return *BufferedIo;

  return *this;
}

bool FileBackend::IsLogOpen() const
{
  return GetActiveIo().IsOpen();
}

FileManagerFactory& FileBackend::GetFactory() const
{
  auto logger = Owner->GetOwner();
  return logger->GetFileManagerFactory();
}

CompressionManagerFactory& FileBackend::GetCompressionFactory() const
{
  auto logger = Owner->GetOwner();
  return logger->GetCompressionManagerFactory();
}

void FileBackend::SubmitCompletedFile(const std::string& file)
{
  if (!GzipCompression || !Compression)
    return;

  FILE_CNT(GlobalCompressionSubmitCalls.fetch_add(1, std::memory_order_relaxed));
  Compression->Submit(file);
}

void FileBackend::ApplyRetention()
{
  RetentionOptions retention;
  retention.MaxFiles = static_cast<std::size_t>(MaxParts);
  retention.MaxAgeMs = RetentionMaxAge;
  retention.MaxTotalSize = RetentionMaxTotalSize;

  if (retention.IsEmpty())
    return;

  FILE_CNT(GlobalRetentionRuns.fetch_add(1, std::memory_order_relaxed));
  std::regex pattern = BuildCleanPattern();
  CleanFiles(pattern, Name, retention);

  if (!ArchivePolicy->IsEnabled())
    return;

  std::string archiveProbe = ArchivePolicy->BuildName(0);
  std::filesystem::path activeDir = std::filesystem::path(Name).parent_path();
  std::filesystem::path archiveDir = std::filesystem::path(archiveProbe).parent_path();
  if (archiveDir.empty() || archiveDir == activeDir)
    return;

  CleanFiles(pattern, archiveProbe, retention);
}

bool FileBackend::CompleteCurrentFile(
  FileCompletionReason reason
  , bool applyRetention
  , std::time_t completedArchiveTime
)
{
  FILE_CNT(GlobalChangePartCalls.fetch_add(1, std::memory_order_relaxed));
  if (reason == FILE_COMPLETION_SIZE_LIMIT)
    FILE_CNT(GlobalSizeLimitCompletionCalls.fetch_add(1, std::memory_order_relaxed));
  else
    FILE_CNT(GlobalTimeLimitCompletionCalls.fetch_add(1, std::memory_order_relaxed));

  const std::string oldName = Name;
  if (completedArchiveTime == 0)
  {
    completedArchiveTime = ArchivePolicy->GetTime();
    if (completedArchiveTime == 0)
      completedArchiveTime = TimeRotationPolicy->GetArchiveTime();
  }

  std::string completedName;

  auto reopenCurrentLog = [this]()
  {
    const bool append = Append;
    Append = true;
    bool result = CreateLog(NameTemplate.c_str());
    Append = append;
    return result;
  };

  if (!oldName.empty() && ArchivePolicy->IsEnabled())
  {
    completedName = ArchivePolicy->TakeName(completedArchiveTime);

    CloseLog();

    std::error_code ec;
    auto dir = std::filesystem::path(completedName).parent_path();
    if (!dir.empty())
    {
      std::filesystem::create_directories(dir, ec);
      if (ec)
      {
        LogmeE(CHINT, "failed to create archive directory: %s", completedName.c_str());
        reopenCurrentLog();
        FILE_CNT(GlobalChangePartFailures.fetch_add(1, std::memory_order_relaxed));
        return false;
      }
    }

    std::filesystem::rename(oldName, completedName, ec);
    if (ec)
    {
      LogmeE(
        CHINT
        , "failed to rename log file to archive: %s -> %s"
        , oldName.c_str()
        , completedName.c_str()
      );
      reopenCurrentLog();
      FILE_CNT(GlobalChangePartFailures.fetch_add(1, std::memory_order_relaxed));
      return false;
    }

    FILE_CNT(GlobalArchivedFiles.fetch_add(1, std::memory_order_relaxed));
  }
  else if (reason == FILE_COMPLETION_SIZE_LIMIT)
  {
    LogmeE(CHINT, "file size limit rotate requested without archive template");
    FILE_CNT(GlobalChangePartFailures.fetch_add(1, std::memory_order_relaxed));
    return false;
  }

  if (!CreateLog(NameTemplate.c_str()))
  {
    LogmeE(CHINT, "failed to create a new log");
    FILE_CNT(GlobalChangePartFailures.fetch_add(1, std::memory_order_relaxed));
    return false;
  }

  if (!ArchivePolicy->IsEnabled() && reason == FILE_COMPLETION_TIME_LIMIT && !oldName.empty() && oldName != Name)
    completedName = oldName;

  ArchivePolicy->SetTime(TimeRotationPolicy->GetArchiveTime());

  if (!completedName.empty())
    SubmitCompletedFile(completedName);

  if (applyRetention)
    ApplyRetention();

  return true;
}

std::regex FileBackend::BuildCleanPattern() const
{
  auto buildPattern = [this](const std::string& source)
  {
    ProcessTemplateParam param(
      static_cast<uint32_t>(TEMPLATE_ALL)
      & ~static_cast<uint32_t>(TEMPLATE_DATE_AND_TIME)
    );

    std::string re = ProcessTemplate(source.c_str(), param);
    re = ReplaceAll(re, "{index}", "{datetime}");

    if (!IsAbsolutePath(re))
      re = Owner->GetOwner()->GetHomeDirectory() + re;

    return ReplaceDatetimePlaceholders(re, ".+");
  };

  std::string re = buildPattern(NameTemplate);
  if (ArchivePolicy->IsEnabled())
    re = "(" + re + ")|(" + ArchivePolicy->BuildCleanPattern() + ")";

  if (GzipCompression)
    re = "(" + re + ")(\\.gz)?";

  return std::regex(re);
}

bool FileBackend::WriteReadyData(std::vector<DataBufferPtr>& data)
{
  FILE_CNT(GlobalWriteReadyCalls.fetch_add(1, std::memory_order_relaxed));
  if (!Queue.TakeReady(data))
  {
    FILE_WRCNT(GlobalWriteReadyEmptyCalls.fetch_add(
      1
      , std::memory_order_relaxed
    ));

    if (QueuedBytes.load(std::memory_order_relaxed) == 0)
    {
      std::lock_guard guard(BufferLock);
      Done.notify_all();
    }

    return false;
  }

  size_t bytes = 0;
  for (auto& b : data)
    bytes += b ? b->Size() : 0;

  FILE_CNT(GlobalWriteBatches.fetch_add(1, std::memory_order_relaxed));
  FILE_CNT(GlobalWriteBatchBytes.fetch_add(bytes, std::memory_order_relaxed));
  FILE_WRCNT(UpdateMaxCounter(GlobalWriteReadyMaxBuffers, data.size()));
  FILE_WRCNT(UpdateMaxCounter(GlobalWriteReadyMaxBytes, bytes));
  {
    std::uint64_t oldMax = GlobalWriteBatchMaxBytes.load(std::memory_order_relaxed);
    while (oldMax < bytes && !GlobalWriteBatchMaxBytes.compare_exchange_weak(
      oldMax
      , bytes
      , std::memory_order_relaxed
      , std::memory_order_relaxed
    ))
    {
    }
  }

  bool ok = true;
  {
    std::lock_guard guard(IoLock);

    if (!ApplySizeLimit(bytes))
    {
      ok = false;
    }
    else
    {

#if !defined(_WIN32) && !defined(__sun__)
    constexpr size_t WRITEV_CHUNK = 256;
    iovec iov[WRITEV_CHUNK];
    size_t iovcnt = 0;

    auto writeVector = [&]()
    {
      if (iovcnt == 0)
        return;

      int rc = iovcnt == 1
        ? FileIo::WriteAll(iov[0].iov_base, iov[0].iov_len)
        : FileIo::WriteAllVector(iov, (int)iovcnt);

      if (rc < 0)
      {
        ok = false;
      }
      else
      {
        CurrentSize += (size_t)rc;
      }

      iovcnt = 0;
    };

    for (auto& b : data)
    {
      if (!b)
        continue;

      FILE_WRCNT(GlobalWriteReadyRawCalls.fetch_add(
        1
        , std::memory_order_relaxed
      ));
      FILE_WRCNT(GlobalWriteReadyRawBytes.fetch_add(
        b->Size()
        , std::memory_order_relaxed
      ));
      FILE_WRCNT(UpdateMaxCounter(GlobalWriteReadyRawMaxBytes, b->Size()));

      iov[iovcnt].iov_base = b->Data();
      iov[iovcnt].iov_len = b->Size();
      ++iovcnt;

      if (iovcnt == WRITEV_CHUNK)
        writeVector();
    }

    writeVector();
#else
    for (auto& b : data)
    {
      if (!b)
        continue;

      FILE_WRCNT(GlobalWriteReadyRawCalls.fetch_add(
        1
        , std::memory_order_relaxed
      ));
      FILE_WRCNT(GlobalWriteReadyRawBytes.fetch_add(
        b->Size()
        , std::memory_order_relaxed
      ));
      FILE_WRCNT(UpdateMaxCounter(GlobalWriteReadyRawMaxBytes, b->Size()));

      int rc = FileIo::WriteAll(b->Data(), b->Size());
      if (rc < 0)
      {
        ok = false;
      }
      else
      {
        CurrentSize += (size_t)rc;
      }
    }
#endif
    }
  }

  Queue.ReportWrite(data.size(), bytes, ok);

  if (ok)
  {
    FILE_CNT(GlobalWrittenBuffers.fetch_add(data.size(), std::memory_order_relaxed));
    FILE_CNT(GlobalWrittenBytes.fetch_add(bytes, std::memory_order_relaxed));
    FILE_CNT(GlobalOutputBytes.fetch_add(bytes, std::memory_order_relaxed));
  }
  else
  {
    FILE_CNT(GlobalWriteErrors.fetch_add(1, std::memory_order_relaxed));
  }

  if (bytes != 0)
    QueuedBytes.fetch_sub(bytes, std::memory_order_relaxed);

  Queue.Recycle(data);

  if (!Queue.HasCurrentDataFlagged() && !Queue.HasReady())
    Queue.TrimFreeBuffersIfIdle();

  if (QueuedBytes.load(std::memory_order_relaxed) == 0)
  {
    std::lock_guard guard(BufferLock);
    Done.notify_all();
  }

  return true;
}

bool FileBackend::PublishCurrentCounted(
  bool& needSignal
  , PublishCurrentSource source
)
{
#if FILE_ENABLE_FLUSH_SOURCE_COUNTERS
  const std::uint64_t oldest = Queue.GetOldestDataTime();
  const std::uint64_t queued = QueuedBytes.load(std::memory_order_relaxed);

  if (source == PublishCurrentSource::FLUSH)
    GlobalPublishFromFlushCalls.fetch_add(1, std::memory_order_relaxed);
  else if (source == PublishCurrentSource::WORKER_IMMEDIATE)
    GlobalPublishFromWorkerImmediateCalls.fetch_add(1, std::memory_order_relaxed);
  else if (source == PublishCurrentSource::WORKER_SHUTDOWN)
    GlobalPublishFromWorkerShutdownCalls.fetch_add(1, std::memory_order_relaxed);
  else if (source == PublishCurrentSource::ON_SHUTDOWN)
    GlobalPublishFromOnShutdownCalls.fetch_add(1, std::memory_order_relaxed);
#endif

  const bool ok = Queue.PublishCurrent(needSignal);

#if FILE_ENABLE_FLUSH_SOURCE_COUNTERS
  if (ok)
  {
    const std::uint64_t now = GetTimeInMillisec64();
    const std::uint64_t age = oldest != 0 && now >= oldest ? now - oldest : 0;

    if (source == PublishCurrentSource::FLUSH)
      GlobalPublishFromFlushSuccess.fetch_add(1, std::memory_order_relaxed);
    else if (source == PublishCurrentSource::WORKER_IMMEDIATE)
      GlobalPublishFromWorkerImmediateSuccess.fetch_add(1, std::memory_order_relaxed);
    else if (source == PublishCurrentSource::WORKER_SHUTDOWN)
      GlobalPublishFromWorkerShutdownSuccess.fetch_add(1, std::memory_order_relaxed);
    else if (source == PublishCurrentSource::ON_SHUTDOWN)
      GlobalPublishFromOnShutdownSuccess.fetch_add(1, std::memory_order_relaxed);

    GlobalPublishCurrentQueuedBytes.fetch_add(queued, std::memory_order_relaxed);
    GlobalPublishCurrentAgeMs.fetch_add(age, std::memory_order_relaxed);
    UpdateMaxCounter(GlobalPublishCurrentMaxQueuedBytes, queued);
    UpdateMaxCounter(GlobalPublishCurrentMaxAgeMs, age);
  }
#endif

  return ok;
}

void FileBackend::UpdateFlushTimeAfterWork()
{
  const auto updateFromOldest = [this](std::uint64_t oldest)
  {
    const std::uint64_t deadline = oldest + FlushAfter;
    const std::uint64_t now = GetTimeInMillisec64();

    if (deadline <= now)
      FlushTime.store(RIGHT_NOW, std::memory_order_relaxed);
    else
      FlushTime.store(deadline, std::memory_order_relaxed);
  };

  std::uint64_t readyTime = Queue.GetOldestReadyDataTime();
  std::uint64_t currentTime = Queue.GetCurrentFirstWriteTimeSnapshot();
  std::uint64_t oldest = readyTime;

  if (oldest == 0 || (currentTime != 0 && currentTime < oldest))
    oldest = currentTime;

  if (oldest != 0)
  {
    updateFromOldest(oldest);
    return;
  }

  if (Queue.HasCurrentDataFlagged())
    return;

  std::uint64_t expected = FlushTime.load(std::memory_order_relaxed);

  while (expected != 0)
  {
    if (FlushTime.compare_exchange_weak(
      expected
      , 0
      , std::memory_order_relaxed
      , std::memory_order_relaxed
    ))
    {
      break;
    }
  }

  readyTime = Queue.GetOldestReadyDataTime();
  currentTime = Queue.GetCurrentFirstWriteTimeSnapshot();
  oldest = readyTime;

  if (oldest == 0 || (currentTime != 0 && currentTime < oldest))
    oldest = currentTime;

  if (oldest != 0)
  {
    updateFromOldest(oldest);
    return;
  }

  if (Queue.HasReady())
  {
    FlushTime.store(RIGHT_NOW, std::memory_order_relaxed);
    return;
  }

  if (Queue.HasCurrentDataFlagged())
  {
    FlushTime.store(
      GetTimeInMillisec64() + FlushAfter
      , std::memory_order_relaxed
    );
    return;
  }
}

bool FileBackend::WorkerFunc()
{
  FILE_CNT(GlobalWorkerRuns.fetch_add(1, std::memory_order_relaxed));
  // Used to limit number of write loops before Done event set if
  // one or more threads call Log() very frequently
  const int maxWriteLoops = 5;

  const bool shutdownRun = ShutdownFlag.load(std::memory_order_relaxed);
  const bool immediateRun = FlushTime.load(std::memory_order_relaxed) == RIGHT_NOW;
  bool forceFlush = shutdownRun || immediateRun;

  if (shutdownRun)
    FILE_CNT(GlobalWorkerShutdownRuns.fetch_add(1, std::memory_order_relaxed));
  else if (immediateRun)
    FILE_CNT(GlobalWorkerImmediateRuns.fetch_add(1, std::memory_order_relaxed));
  else
    FILE_CNT(GlobalWorkerTimedRuns.fetch_add(1, std::memory_order_relaxed));

  FILE_WRCNT(int writeLoops = 0);
  for (int i = 0; i < maxWriteLoops; i++)
  {
    FILE_WRCNT(GlobalWorkerWriteLoopIterations.fetch_add(
      1
      , std::memory_order_relaxed
    ));
    FILE_WRCNT(writeLoops++);

    if (WriteReadyData(ReadyData))
    {
      forceFlush = false;
      continue;
    }

    if (forceFlush)
    {
      bool needSignal = false;
      FILE_WRCNT(GlobalWorkerPublishCurrentCalls.fetch_add(
        1
        , std::memory_order_relaxed
      ));
      if (PublishCurrentCounted(
        needSignal
        , shutdownRun
          ? PublishCurrentSource::WORKER_SHUTDOWN
          : PublishCurrentSource::WORKER_IMMEDIATE
      ))
      {
        FILE_WRCNT(GlobalWorkerPublishCurrentSuccess.fetch_add(
          1
          , std::memory_order_relaxed
        ));
        (void)needSignal;
        forceFlush = false;
        continue;
      }
    }
    else
    {
      break;
    }

    FILE_WRCNT(GlobalWorkerBreaks.fetch_add(1, std::memory_order_relaxed));
    break;
  }

  FILE_WRCNT(UpdateMaxCounter(GlobalWorkerWriteLoopMaxIterations, writeLoops));

  UpdateFlushTimeAfterWork();
  return !ShutdownFlag.load(std::memory_order_relaxed);
}

void FileBackend::OnShutdown()
{
  FILE_CNT(GlobalShutdownCalls.fetch_add(1, std::memory_order_relaxed));
  while (QueuedBytes.load(std::memory_order_relaxed) != 0)
  {
    if (!WriteReadyData(ReadyData))
    {
      bool needSignal = false;
      if (!PublishCurrentCounted(needSignal, PublishCurrentSource::ON_SHUTDOWN))
        break;

      (void)needSignal;
    }
  }

  ShutdownCalled.store(true, std::memory_order_relaxed);
  Shutdown.notify_all();
}
