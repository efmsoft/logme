#include <cassert>
#include <chrono>
#include <filesystem>
#include <regex>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
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
  std::atomic<std::uint64_t> GlobalCreateLogCalls(0);
  std::atomic<std::uint64_t> GlobalCreateLogFailures(0);
  std::atomic<std::uint64_t> GlobalChangePartCalls(0);
  std::atomic<std::uint64_t> GlobalChangePartFailures(0);
  std::atomic<std::uint64_t> GlobalShutdownCalls(0);
}

size_t FileBackend::MaxSizeDefault = FileBackend::MAX_SIZE_DEFAULT;
size_t FileBackend::QueueSizeLimitDefault = FileBackend::QUEUE_SIZE_LIMIT;

FileBackend::FileBackend(ChannelPtr owner)
  : Backend(owner, TYPE_ID)
  , Append(true)
  , MaxSize(MaxSizeDefault)
  , CurrentSize(0)
  , QueueSizeLimit(QueueSizeLimitDefault)
  , Registered(false)
  , ShutdownFlag(false)
  , ShutdownCalled(owner == nullptr)
  , FlushTime(0)
  , Queue(Owner.get(), []
    {
      BufferQueue::Options o;
      o.BufferSize = QUEUE_BUFFER_SIZE;
      o.MaxTotalBuffers = MAX_TOTAL_BUFFERS;
      return o;
    }())
  , QueuedBytes(0)
  , DailyRotation(false)
  , MaxParts(2)
{
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
  if (size < 4ULL * 1024)
    size = 4ULL * 1024;

  QueueSizeLimitDefault = size;
}

std::string FileBackend::FormatDetails()
{
  std::string out = NameTemplate;
  if (Append)
    out += " APPEND";

  return out;
}

BackendConfigPtr FileBackend::CreateConfig()
{
  return std::make_shared<FileBackendConfig>();
}

void FileBackend::Flush()
{
  FILE_CNT(GlobalFlushWaitCalls.fetch_add(1, std::memory_order_relaxed));
  for (;;)
  {
    bool needSignal = false;
    if (!Queue.PublishCurrent(needSignal))
      break;
  }

  RequestFlush();

  std::unique_lock locker(BufferLock);
  Done.wait(locker, [this]() { return QueuedBytes.load(std::memory_order_relaxed) == 0; });
}

void FileBackend::Freeze()
{
  ShutdownFlag.store(true, std::memory_order_relaxed);
  Backend::Freeze();

  RequestFlush();
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
  NameTemplate = v;

  ProcessTemplateParam param;
  std::string name = ProcessTemplate(v, param);

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

  if (!Open(Append))
  {
    FILE_CNT(GlobalCreateLogFailures.fetch_add(1, std::memory_order_relaxed));
    return false;
  }

  if (!Append)
  {
    CurrentSize = 0;
    return true;
  }

  auto rc = Seek(0, SEEK_END);
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
  Close();
}

bool FileBackend::TestFileInUse(const std::string& file) const
{
  if (File == -1)
    return false;

  return Name == file;
}

size_t FileBackend::GetSize()
{
  if (File == -1)
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
  if (File == -1 || ShutdownFlag.load(std::memory_order_relaxed))
    return;

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

  FileIo::TruncateToMaxSize(MaxSize);

  auto rc = Seek(0, SEEK_END);
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

  if (File == -1 || ShutdownFlag.load(std::memory_order_relaxed))
    return;

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

  bool needSignal = false;
  bool firstData = false;
  uint64_t now = uint64_t(GetTimeInMillisec());

  if (Queue.Append(text, add, now, needSignal, firstData) == false)
    return;

  (void)needSignal;

  FILE_CNT(GlobalAppendCalls.fetch_add(1, std::memory_order_relaxed));
  FILE_CNT(GlobalInputBytes.fetch_add(add, std::memory_order_relaxed));

  size_t queued = QueuedBytes.fetch_add(add, std::memory_order_relaxed) + add;
  size_t usedBuffers = Queue.GetUsedBuffers();
  uint64_t flushTime = FlushTime.load(std::memory_order_relaxed);

  if (usedBuffers >= FLUSH_PRESSURE_BUFFERS || queued >= QueueSizeLimit)
  {
    if (flushTime != RIGHT_NOW)
      RequestFlush();
  }
  else if (firstData)
  {
    uint64_t when = now + FLUSH_AFTER;
    if (flushTime == 0 || (flushTime != RIGHT_NOW && flushTime > when))
      RequestFlush(when);
  }
}

void FileBackend::RequestFlush(uint64_t when)
{
  const bool immediate = when == RIGHT_NOW;
  FILE_CNT(GlobalFlushRequests.fetch_add(1, std::memory_order_relaxed));
  if (immediate)
    FILE_CNT(GlobalImmediateFlushRequests.fetch_add(1, std::memory_order_relaxed));
  else
    FILE_CNT(GlobalScheduledFlushRequests.fetch_add(1, std::memory_order_relaxed));

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

FileManagerFactory& FileBackend::GetFactory() const
{
  auto logger = Owner->GetOwner();
  return logger->GetFileManagerFactory();
}

bool FileBackend::WriteReadyData()
{
  FILE_CNT(GlobalWriteReadyCalls.fetch_add(1, std::memory_order_relaxed));
  std::vector<DataBufferPtr> data;
  if (!Queue.TakeReady(data))
  {
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

void FileBackend::UpdateFlushTimeAfterWork()
{
  uint64_t oldest = Queue.GetOldestDataTime();

  if (oldest == 0)
  {
    FlushTime.store(0, std::memory_order_relaxed);
    return;
  }

  uint64_t deadline = oldest + FLUSH_AFTER;
  uint64_t now = uint64_t(GetTimeInMillisec());

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

  for (int i = 0; i < maxWriteLoops; i++)
  {
    if (WriteReadyData())
    {
      forceFlush = false;
      continue;
    }

    if (forceFlush)
    {
      bool needSignal = false;
      if (Queue.PublishCurrent(needSignal))
      {
        (void)needSignal;
        forceFlush = false;
        continue;
      }
    }
    else
    {
      bool needSignal = false;
      if (Queue.PublishCurrent(needSignal))
      {
        (void)needSignal;
        continue;
      }
    }

    break;
  }

  UpdateFlushTimeAfterWork();
  return !ShutdownFlag.load(std::memory_order_relaxed);
}

void FileBackend::OnShutdown()
{
  FILE_CNT(GlobalShutdownCalls.fetch_add(1, std::memory_order_relaxed));
  while (QueuedBytes.load(std::memory_order_relaxed) != 0)
  {
    if (!WriteReadyData())
    {
      bool needSignal = false;
      if (!Queue.PublishCurrent(needSignal))
        break;

      (void)needSignal;
    }
  }

  ShutdownCalled.store(true, std::memory_order_relaxed);
  Shutdown.notify_all();
}
