#include <cassert>
#include <chrono>
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

#include <Logme/File/FileManagerFactory.h>
#include <Logme/Logger.h>
#include <Logme/Logme.h>
#include <Logme/Template.h>
#include <Logme/Time/datetime.h> 
#include <Logme/Types.h>

#include "../File/CleanOldest.h"

using namespace std::chrono_literals;

#ifndef _WIN32
#include <sys/file.h>
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
  std::atomic<std::uint64_t> GlobalShutdownCalls(0);
}

size_t FileBackend::MaxSizeDefault = FileBackend::MAX_SIZE_DEFAULT;
size_t FileBackend::QueueSizeLimitDefault = FileBackend::QUEUE_SIZE_LIMIT;
uint64_t FileBackend::FlushAfterDefault = FileBackend::FLUSH_AFTER_DEFAULT;
std::atomic<size_t> FileBackend::DataBufferCacheLimit(8);

FileBackend::FileBackend(ChannelPtr owner)
  : MemoryTrackedBackend(owner, TYPE_ID)
  , Append(true)
  , MaxSize(MaxSizeDefault)
  , CurrentSize(0)
  , QueueSizeLimit(QueueSizeLimitDefault)
  , FlushAfter(FlushAfterDefault)
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
      o.BufferSize = QUEUE_BUFFER_SIZE;
      o.MaxTotalBuffers = MAX_TOTAL_BUFFERS;
      o.CacheContext = owner.get();
      o.TakeCachedBuffer = &FileBackend::TakeCachedDataBuffer;
      o.ReturnCachedBuffer = &FileBackend::ReturnCachedDataBuffer;
      return o;
    }()
    , GetMemoryUsageTracker()
  )
  , QueuedBytes(0)
  , DailyRotation(false)
  , MaxParts(2)
  , BufferedIo(std::make_unique<BufferedFileIo>(this))
{
  SetAsync(true);
  ReadyData.reserve(MAX_TOTAL_BUFFERS);
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

size_t FileBackend::GetQueueSizeLimitDefault()
{
  return QueueSizeLimitDefault;
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
  if (GetDataBufferCacheLimit() == 0 || !context)
    return nullptr;

  Channel* channel = static_cast<Channel*>(context);
  Logger* logger = channel->GetOwner();

  if (!logger)
    return nullptr;

  return logger->GetFileManagerFactory().TakeDataBuffer(
    memoryTracker
    , capacity
  );
}

bool FileBackend::ReturnCachedDataBuffer(void* context, DataBufferPtr buffer)
{
  if (GetDataBufferCacheLimit() == 0 || !context || !buffer)
    return false;

  Channel* channel = static_cast<Channel*>(context);
  Logger* logger = channel->GetOwner();

  if (!logger)
    return false;

  return logger->GetFileManagerFactory().ReturnDataBuffer(
    std::move(buffer)
    , GetDataBufferCacheLimit()
  );
}

std::string FileBackend::FormatDetails()
{
  std::ostringstream os;
  os << NameTemplate;
  os << (Append ? " APPEND" : " OVERWRITE");
  os << " MaxSize=" << MaxSize;
  os << " DailyRotation=" << (DailyRotation ? "YES" : "NO");
  os << " MaxParts=" << MaxParts;
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

  SetAsync(p->Async);
  SetAppend(p->DailyRotation ? true : p->Append);
  SetMaxSize(p->MaxSize);

  DailyRotation = p->DailyRotation;
  MaxParts = p->MaxParts;
  
  Day.UpdateDayBoundaries();

  NameTemplate = p->Filename;
  return ChangePart();
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
  
  auto dir = std::filesystem::path(Name).parent_path();
  if (!dir.empty())
  {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);

    if (ec)
    {
      FILE_CNT(GlobalCreateLogFailures.fetch_add(1, std::memory_order_relaxed));
      return false;
    }
  }

  if (!GetActiveIo().Open(Append))
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

bool FileBackend::ChangePart()
{
  FILE_CNT(GlobalChangePartCalls.fetch_add(1, std::memory_order_relaxed));
  if (!CreateLog(NameTemplate.c_str()))
  {
    LogmeE(CHINT, "failed to create a new log");
    FILE_CNT(GlobalChangePartFailures.fetch_add(1, std::memory_order_relaxed));
    return false;
  }

  ProcessTemplateParam param(static_cast<uint32_t>(TEMPLATE_ALL) & ~static_cast<uint32_t>(TEMPLATE_DATE_AND_TIME));
  std::string re = ProcessTemplate(NameTemplate.c_str(), param);

  re = ReplaceDatetimePlaceholders(re, ".+");
  CleanFiles(std::regex(re), Name, MaxParts);

  return true;
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

  if (DailyRotation && Day.IsSameDayCached() == false)
  {
    if (!ChangePart())
      return;
  }

  int nc;
  const char* buffer = context.Apply(Owner, Owner->GetFlags(), nc);
  AppendStringInternal(buffer, nc);
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
    Truncate();

    int rc = GetActiveIo().WriteRaw(text, add);
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

  if (Queue.Append(text, add, needSignal, firstData) == false)
    return;

  (void)needSignal;

  FILE_CNT(GlobalAppendCalls.fetch_add(1, std::memory_order_relaxed));
  FILE_CNT(GlobalInputBytes.fetch_add(add, std::memory_order_relaxed));

  size_t queued = QueuedBytes.fetch_add(add, std::memory_order_relaxed) + add;
  size_t usedBuffers = Queue.GetUsedBuffers();
  uint64_t flushTime = FlushTime.load(std::memory_order_relaxed);

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
      uint64_t firstWriteTime = GetTimeInMillisec64();
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
      uint64_t firstWriteTime = GetTimeInMillisec64();
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
  if (!GetAsync() && BufferedIo)
    return *BufferedIo;

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

    Truncate();

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

      int rc = FileIo::WriteRaw(b->Data(), b->Size());
      if (rc < 0)
      {
        ok = false;
      }
      else
      {
        CurrentSize += (size_t)rc;
      }
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
  uint64_t oldest = Queue.GetOldestDataTime();

  if (oldest == 0)
  {
    FlushTime.store(0, std::memory_order_relaxed);
    return;
  }

  uint64_t deadline = oldest + FlushAfter;
  uint64_t now = GetTimeInMillisec64();

  if (deadline <= now)
    FlushTime.store(RIGHT_NOW, std::memory_order_relaxed);
  else
    FlushTime.store(deadline, std::memory_order_relaxed);
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
