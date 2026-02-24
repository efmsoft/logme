#include <Logme/Buffer/BufferQueue.h>

using namespace Logme;

BufferQueue::BufferQueue(const Options& options)
  : OptionsValue(options)
  , TotalBuffers(0)
  , AdaptiveFreeLimit(options.BaseFreeLimit)
  , Signaled(false)
{
  Current.reset(new DataBuffer(options.BufferSize));
  TotalBuffers = 1;
  BFW_CNT(Counters.AllocatedBuffers += 1);
}

bool BufferQueue::Append(const char* p, std::size_t cb, bool& needSignal)
{
  needSignal = false;

  if (cb == 0)
  {
    return true;
  }

  std::lock_guard<std::mutex> guard(Lock);

  BFW_CNT(Counters.Appends += 1);
  BFW_CNT(Counters.AppendBytes += cb);

  if (cb > OptionsValue.BufferSize)
  {
    BFW_CNT(Counters.DroppedAppends += 1);
    BFW_CNT(Counters.DroppedBytes += cb);
    return false;
  }

  if (Current->CanAppend(cb))
  {
    Current->Append(p, cb);
    return true;
  }

  EnqueueReadyLocked(std::move(Current), needSignal);

  bool allocatedBecauseNoFree = false;

  Current = AcquireBufferLocked();
  if (!Current)
  {
    BFW_CNT(Counters.DroppedAppends += 1);
    BFW_CNT(Counters.DroppedBytes += cb);
    return false;
  }

  if (FreeList.empty())
  {
    allocatedBecauseNoFree = true;
  }

  MaybeGrowAdaptiveLocked(allocatedBecauseNoFree);

  Current->Append(p, cb);
  return true;
}

bool BufferQueue::SwapReady(std::vector<DataBufferPtr>& out)
{
  out.clear();

  std::lock_guard<std::mutex> guard(Lock);

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
  std::lock_guard<std::mutex> guard(Lock);

  for (auto& buffer : buffers)
  {
    if (!buffer)
    {
      continue;
    }

    buffer->Reset();
    ReleaseBufferLocked(std::move(buffer));
  }

  buffers.clear();

  DecayLocked();
}

bool BufferQueue::FlushCurrent(bool& needSignal)
{
  needSignal = false;

  std::lock_guard<std::mutex> guard(Lock);

  if (!Current)
  {
    return false;
  }

  if (Current->Size() == 0)
  {
    return false;
  }

  EnqueueReadyLocked(std::move(Current), needSignal);

  Current = AcquireBufferLocked();
  if (!Current)
  {
    Current.reset(new DataBuffer(OptionsValue.BufferSize));
    TotalBuffers += 1;
    BFW_CNT(Counters.AllocatedBuffers += 1);
  }

  return true;
}

void BufferQueue::ReportWrite(std::size_t buffers, std::size_t bytes, bool ok)
{
  std::lock_guard<std::mutex> guard(Lock);

  if (ok)
  {
    BFW_CNT(Counters.WrittenBuffers += buffers);
    BFW_CNT(Counters.WrittenBytes += bytes);
    return;
  }

  BFW_CNT(Counters.WriteErrors += 1);
}

BufferCounters BufferQueue::GetCounters() const
{
  std::lock_guard<std::mutex> guard(Lock);
  return Counters;
}

std::size_t BufferQueue::GetTotalBuffers() const
{
  std::lock_guard<std::mutex> guard(Lock);
  return TotalBuffers;
}

std::size_t BufferQueue::GetAdaptiveFreeLimit() const
{
  std::lock_guard<std::mutex> guard(Lock);
  return AdaptiveFreeLimit;
}

DataBufferPtr BufferQueue::AcquireBufferLocked()
{
  if (!FreeList.empty())
  {
    DataBufferPtr buffer = std::move(FreeList.front());
    FreeList.pop_front();
    return buffer;
  }

  if (OptionsValue.MaxTotalBuffers != 0 &&
      TotalBuffers >= OptionsValue.MaxTotalBuffers)
  {
    return nullptr;
  }

  TotalBuffers += 1;

  BFW_CNT(Counters.AllocatedBuffers += 1);

  return DataBufferPtr(
    new DataBuffer(OptionsValue.BufferSize)
  );
}

void BufferQueue::ReleaseBufferLocked(DataBufferPtr buffer)
{
  FreeList.push_back(std::move(buffer));

  while (FreeList.size() > AdaptiveFreeLimit)
  {
    FreeList.pop_front();
    TotalBuffers -= 1;
    BFW_CNT(Counters.DeletedBuffers += 1);
  }
}

void BufferQueue::EnqueueReadyLocked(DataBufferPtr buffer, bool& needSignal)
{
  if (!buffer)
  {
    return;
  }

  if (buffer->Size() == 0)
  {
    buffer->Reset();
    ReleaseBufferLocked(std::move(buffer));
    return;
  }

  ReadyList.push_back(std::move(buffer));

  BFW_CNT(Counters.EnqueuedBuffers += 1);

  if (!Signaled)
  {
    Signaled = true;
    needSignal = true;
    BFW_CNT(Counters.SignalsSent += 1);
  }
}

void BufferQueue::MaybeGrowAdaptiveLocked(bool allocatedBecauseNoFree)
{
  if (!allocatedBecauseNoFree)
  {
    return;
  }

  std::size_t cap = 0;

  if (OptionsValue.MaxAdaptiveFreeLimit != 0)
  {
    cap = OptionsValue.MaxAdaptiveFreeLimit;
  }
  else if (OptionsValue.MaxTotalBuffers != 0)
  {
    cap = OptionsValue.MaxTotalBuffers;
  }

  if (cap != 0 && AdaptiveFreeLimit >= cap)
  {
    return;
  }

  AdaptiveFreeLimit += 1;
}

void BufferQueue::DecayLocked()
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
    BFW_CNT(Counters.DeletedBuffers += 1);
  }
}