#include <Logme/Buffer/BufferQueue.h>
#include <Logme/Channel.h>

using namespace Logme;

namespace
{
  std::atomic<std::uint64_t> GlobalAppends(0);
  std::atomic<std::uint64_t> GlobalAppendBytes(0);
  std::atomic<std::uint64_t> GlobalEnqueuedBuffers(0);
  std::atomic<std::uint64_t> GlobalWrittenBuffers(0);
  std::atomic<std::uint64_t> GlobalWrittenBytes(0);
  std::atomic<std::uint64_t> GlobalAllocatedBuffers(0);
  std::atomic<std::uint64_t> GlobalDeletedBuffers(0);
  std::atomic<std::uint64_t> GlobalDroppedAppends(0);
  std::atomic<std::uint64_t> GlobalDroppedBytes(0);
  std::atomic<std::uint64_t> GlobalWriteErrors(0);
  std::atomic<std::uint64_t> GlobalSignalsSent(0);
}

BufferQueue::BufferQueue(Channel* owner, const Options& options)
  : Owner(owner)
  , OptionsValue(options)
  , HasCurrentDataFlag(false)
  , UsedBuffersCount(1)
  , TotalBuffers(0)
  , AdaptiveFreeLimit(options.BaseFreeLimit)
  , Signaled(false)
  , Appends(0)
  , AppendBytes(0)
  , EnqueuedBuffers(0)
  , WrittenBuffers(0)
  , WrittenBytes(0)
  , AllocatedBuffers(0)
  , DeletedBuffers(0)
  , DroppedAppends(0)
  , DroppedBytes(0)
  , WriteErrors(0)
  , SignalsSent(0)
{
  Current.reset(new DataBuffer(options.BufferSize));
  TotalBuffers = 1;
  CountAllocated();
}

bool BufferQueue::Append(
  const char* p
  , std::size_t cb
  , std::uint64_t now
  , bool& needSignal
  , bool& firstData
)
{
  // Must be called with Owner->GetDataLock() already held!!!!!

  needSignal = false;
  firstData = false;

  if (cb == 0)
    return true;

  if (cb > OptionsValue.BufferSize)
  {
    CountDropped(cb);
    return false;
  }

  if (Current->CanAppend(cb))
  {
    firstData = Current->Size() == 0;
    if (firstData)
    {
      Current->SetFirstWriteTime(now);
      HasCurrentDataFlag.store(true, std::memory_order_relaxed);
    }

    Current->Append(p, cb);
    CountAppended(cb);
    return true;
  }

  DataBufferPtr replacement = TryTakeFreeBuffer();
  bool allocatedBecauseNoFree = false;

  if (!replacement)
  {
    replacement = TryCreateBuffer();
    allocatedBecauseNoFree = replacement != nullptr;
  }

  if (!replacement)
  {
    CountDropped(cb);
    return false;
  }

  DataBufferPtr readyBuffer;

  if (Current->CanAppend(cb))
  {
    firstData = Current->Size() == 0;
    if (firstData)
    {
      Current->SetFirstWriteTime(now);
      HasCurrentDataFlag.store(true, std::memory_order_relaxed);
    }

    Current->Append(p, cb);
    ReleaseBuffer(std::move(replacement));
    CountAppended(cb);
    return true;
  }

  replacement->Reset();
  replacement->SetSeenOnSoftFlush(false);
  readyBuffer = std::move(Current);

  Current = std::move(replacement);
  Current->SetFirstWriteTime(now);
  Current->Append(p, cb);
  HasCurrentDataFlag.store(true, std::memory_order_relaxed);
  firstData = true;

  MaybeGrowAdaptive(allocatedBecauseNoFree);
  EnqueueReady(std::move(readyBuffer), needSignal);

  CountAppended(cb);
  return true;
}

bool BufferQueue::TakeReady(std::vector<DataBufferPtr>& out)
{
  out.clear();

  std::lock_guard guard(ReadyLock);

  if (ReadyList.empty())
  {
    Signaled = false;
    return false;
  }

  out.reserve(ReadyList.size());

  while (!ReadyList.empty())
  {
    out.push_back(std::move(ReadyList.front()));
    ReadyList.pop_front();
  }

  Signaled = false;
  return true;
}

void BufferQueue::Recycle(std::vector<DataBufferPtr>& buffers)
{
  std::lock_guard guard(FreeLock);

  for (auto& buffer : buffers)
  {
    if (!buffer)
      continue;

    buffer->Reset();
    FreeList.push_back(std::move(buffer));
    UsedBuffersCount.fetch_sub(1, std::memory_order_relaxed);
  }

  buffers.clear();
}

BufferQueue::SoftFlushState BufferQueue::PrepareSoftFlushCurrent(DataBuffer* &expected)
{
  expected = nullptr;

  std::lock_guard guard(Owner->GetDataLock());

  if (!Current || Current->Size() == 0)
    return SoftFlushState::EMPTY;

  if (!Current->SeenOnSoftFlush())
  {
    Current->SetSeenOnSoftFlush(true);
    return SoftFlushState::MARKED;
  }

  expected = Current.get();
  return SoftFlushState::PUBLISH;
}

bool BufferQueue::PublishCurrent(bool& needSignal)
{
  needSignal = false;

  DataBufferPtr replacement = TryTakeFreeBuffer();
  bool allocatedBecauseNoFree = false;

  if (!replacement)
  {
    replacement = TryCreateBuffer();
    allocatedBecauseNoFree = replacement != nullptr;
  }

  if (!replacement)
    return false;

  DataBufferPtr readyBuffer;

  {
    std::lock_guard guard(Owner->GetDataLock());

    if (!Current || Current->Size() == 0)
    {
      ReleaseBuffer(std::move(replacement));
      return false;
    }

    replacement->Reset();
    replacement->SetSeenOnSoftFlush(false);
    readyBuffer = std::move(Current);
    Current = std::move(replacement);
    HasCurrentDataFlag.store(false, std::memory_order_relaxed);
  }

  MaybeGrowAdaptive(allocatedBecauseNoFree);
  EnqueueReady(std::move(readyBuffer), needSignal);
  return true;
}

bool BufferQueue::PublishCurrentIfMatches(DataBuffer* expected, bool& needSignal)
{
  needSignal = false;

  if (!expected)
    return false;

  DataBufferPtr replacement = TryTakeFreeBuffer();
  bool allocatedBecauseNoFree = false;

  if (!replacement)
  {
    replacement = TryCreateBuffer();
    allocatedBecauseNoFree = replacement != nullptr;
  }

  if (!replacement)
  {
    return false;
  }

  DataBufferPtr readyBuffer;

  {
    std::lock_guard guard(Owner->GetDataLock());

    if (!Current || Current.get() != expected || Current->Size() == 0)
    {
      ReleaseBuffer(std::move(replacement));
      return false;
    }

    replacement->Reset();
    replacement->SetSeenOnSoftFlush(false);
    readyBuffer = std::move(Current);
    Current = std::move(replacement);
    HasCurrentDataFlag.store(false, std::memory_order_relaxed);
  }

  MaybeGrowAdaptive(allocatedBecauseNoFree);
  EnqueueReady(std::move(readyBuffer), needSignal);
  return true;
}

void BufferQueue::ReportWrite(std::size_t buffers, std::size_t bytes, bool ok)
{
  CountWritten(buffers, bytes, ok);
}

bool BufferQueue::HasReady() const
{
  std::lock_guard guard(ReadyLock);
  return !ReadyList.empty();
}

bool BufferQueue::HasCurrentData() const
{
  std::lock_guard guard(Owner->GetDataLock());

  if (!Current)
    return false;

  return Current->Size() != 0;
}

bool BufferQueue::HasCurrentDataFlagged() const
{
  return HasCurrentDataFlag.load(std::memory_order_relaxed);
}

std::size_t BufferQueue::GetUsedBuffers() const
{
  return UsedBuffersCount.load(std::memory_order_relaxed);
}

void BufferQueue::TrimFreeBuffersIfIdle()
{
  std::lock_guard guard(FreeLock);
  DecayFreeLocked();
}

std::uint64_t BufferQueue::GetOldestDataTime() const
{
  std::uint64_t readyTime = 0;
  std::uint64_t currentTime = 0;

  {
    std::lock_guard guard(ReadyLock);

    if (!ReadyList.empty() && ReadyList.front())
      readyTime = ReadyList.front()->FirstWriteTime();
  }

  {
    std::lock_guard guard(Owner->GetDataLock());

    if (Current && Current->Size() != 0)
      currentTime = Current->FirstWriteTime();
  }

  if (readyTime == 0)
    return currentTime;

  if (currentTime == 0)
    return readyTime;

  return readyTime < currentTime ? readyTime : currentTime;
}

BufferCounters BufferQueue::GetCounters() const
{
  BufferCounters out;
  out.Appends = Appends.load(std::memory_order_relaxed);
  out.AppendBytes = AppendBytes.load(std::memory_order_relaxed);
  out.EnqueuedBuffers = EnqueuedBuffers.load(std::memory_order_relaxed);
  out.WrittenBuffers = WrittenBuffers.load(std::memory_order_relaxed);
  out.WrittenBytes = WrittenBytes.load(std::memory_order_relaxed);
  out.AllocatedBuffers = AllocatedBuffers.load(std::memory_order_relaxed);
  out.DeletedBuffers = DeletedBuffers.load(std::memory_order_relaxed);
  out.DroppedAppends = DroppedAppends.load(std::memory_order_relaxed);
  out.DroppedBytes = DroppedBytes.load(std::memory_order_relaxed);
  out.WriteErrors = WriteErrors.load(std::memory_order_relaxed);
  out.SignalsSent = SignalsSent.load(std::memory_order_relaxed);
  return out;
}

BufferCounters BufferQueue::GetGlobalCounters()
{
  BufferCounters out;
  out.Appends = GlobalAppends.load(std::memory_order_relaxed);
  out.AppendBytes = GlobalAppendBytes.load(std::memory_order_relaxed);
  out.EnqueuedBuffers = GlobalEnqueuedBuffers.load(std::memory_order_relaxed);
  out.WrittenBuffers = GlobalWrittenBuffers.load(std::memory_order_relaxed);
  out.WrittenBytes = GlobalWrittenBytes.load(std::memory_order_relaxed);
  out.AllocatedBuffers = GlobalAllocatedBuffers.load(std::memory_order_relaxed);
  out.DeletedBuffers = GlobalDeletedBuffers.load(std::memory_order_relaxed);
  out.DroppedAppends = GlobalDroppedAppends.load(std::memory_order_relaxed);
  out.DroppedBytes = GlobalDroppedBytes.load(std::memory_order_relaxed);
  out.WriteErrors = GlobalWriteErrors.load(std::memory_order_relaxed);
  out.SignalsSent = GlobalSignalsSent.load(std::memory_order_relaxed);
  return out;
}

DataBufferPtr BufferQueue::TryTakeFreeBuffer()
{
  std::lock_guard guard(FreeLock);

  if (FreeList.empty())
    return nullptr;

  DataBufferPtr buffer = std::move(FreeList.front());
  FreeList.pop_front();
  UsedBuffersCount.fetch_add(1, std::memory_order_relaxed);
  return buffer;
}

DataBufferPtr BufferQueue::TryCreateBuffer()
{
  std::lock_guard createGuard(CreateLock);

  {
    std::lock_guard freeGuard(FreeLock);

    if (OptionsValue.MaxTotalBuffers != 0
        && TotalBuffers >= OptionsValue.MaxTotalBuffers)
    {
      return nullptr;
    }
  }

  DataBufferPtr buffer(new DataBuffer(OptionsValue.BufferSize));

  {
    std::lock_guard freeGuard(FreeLock);

    if (OptionsValue.MaxTotalBuffers != 0
        && TotalBuffers >= OptionsValue.MaxTotalBuffers)
    {
      return nullptr;
    }

    TotalBuffers += 1;
    UsedBuffersCount.fetch_add(1, std::memory_order_relaxed);
  }

  CountAllocated();
  return buffer;
}

void BufferQueue::ReleaseBuffer(DataBufferPtr buffer)
{
  if (!buffer)
    return;

  buffer->Reset();

  std::lock_guard guard(FreeLock);
  FreeList.push_back(std::move(buffer));
  UsedBuffersCount.fetch_sub(1, std::memory_order_relaxed);
  DecayFreeLocked();
}

void BufferQueue::EnqueueReady(DataBufferPtr buffer, bool& needSignal)
{
  if (!buffer)
    return;

  if (buffer->Size() == 0)
  {
    ReleaseBuffer(std::move(buffer));
    return;
  }

  std::lock_guard guard(ReadyLock);

  ReadyList.push_back(std::move(buffer));
  CountEnqueued();

  if (!Signaled)
  {
    Signaled = true;
    needSignal = true;
    CountSignal();
  }
}

void BufferQueue::MaybeGrowAdaptive(bool allocatedBecauseNoFree)
{
  if (!allocatedBecauseNoFree)
    return;

  std::size_t cap = 0;
  std::lock_guard guard(FreeLock);

  if (OptionsValue.MaxAdaptiveFreeLimit != 0)
  {
    cap = OptionsValue.MaxAdaptiveFreeLimit;
  }
  else if (OptionsValue.MaxTotalBuffers != 0)
  {
    cap = OptionsValue.MaxTotalBuffers;
  }

  if (cap != 0 && AdaptiveFreeLimit >= cap)
    return;

  AdaptiveFreeLimit += 1;
}

void BufferQueue::DecayFreeLocked()
{
  while (
    AdaptiveFreeLimit > OptionsValue.BaseFreeLimit
    && FreeList.size() <= AdaptiveFreeLimit - 1
  )
  {
    AdaptiveFreeLimit -= 1;
  }

  while (FreeList.size() > AdaptiveFreeLimit)
  {
    FreeList.pop_front();
    TotalBuffers -= 1;
    CountDeleted();
  }
}

void BufferQueue::CountAppended(std::size_t cb)
{
  BFW_CNT(Appends.fetch_add(1, std::memory_order_relaxed));
  BFW_CNT(AppendBytes.fetch_add(cb, std::memory_order_relaxed));
  FILE_CNT(GlobalAppends.fetch_add(1, std::memory_order_relaxed));
  FILE_CNT(GlobalAppendBytes.fetch_add(cb, std::memory_order_relaxed));
}

void BufferQueue::CountDropped(std::size_t cb)
{
  BFW_CNT(DroppedAppends.fetch_add(1, std::memory_order_relaxed));
  BFW_CNT(DroppedBytes.fetch_add(cb, std::memory_order_relaxed));
  FILE_CNT(GlobalDroppedAppends.fetch_add(1, std::memory_order_relaxed));
  FILE_CNT(GlobalDroppedBytes.fetch_add(cb, std::memory_order_relaxed));
}

void BufferQueue::CountAllocated()
{
  BFW_CNT(AllocatedBuffers.fetch_add(1, std::memory_order_relaxed));
  FILE_CNT(GlobalAllocatedBuffers.fetch_add(1, std::memory_order_relaxed));
}

void BufferQueue::CountDeleted()
{
  BFW_CNT(DeletedBuffers.fetch_add(1, std::memory_order_relaxed));
  FILE_CNT(GlobalDeletedBuffers.fetch_add(1, std::memory_order_relaxed));
}

void BufferQueue::CountEnqueued()
{
  BFW_CNT(EnqueuedBuffers.fetch_add(1, std::memory_order_relaxed));
  FILE_CNT(GlobalEnqueuedBuffers.fetch_add(1, std::memory_order_relaxed));
}

void BufferQueue::CountWritten(std::size_t buffers, std::size_t bytes, bool ok)
{
  if (ok)
  {
    BFW_CNT(WrittenBuffers.fetch_add(buffers, std::memory_order_relaxed));
    BFW_CNT(WrittenBytes.fetch_add(bytes, std::memory_order_relaxed));
    FILE_CNT(GlobalWrittenBuffers.fetch_add(buffers, std::memory_order_relaxed));
    FILE_CNT(GlobalWrittenBytes.fetch_add(bytes, std::memory_order_relaxed));
    return;
  }

  BFW_CNT(WriteErrors.fetch_add(1, std::memory_order_relaxed));
  FILE_CNT(GlobalWriteErrors.fetch_add(1, std::memory_order_relaxed));
}

void BufferQueue::CountSignal()
{
  BFW_CNT(SignalsSent.fetch_add(1, std::memory_order_relaxed));
  FILE_CNT(GlobalSignalsSent.fetch_add(1, std::memory_order_relaxed));
}
