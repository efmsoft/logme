#pragma once

#include <Logme/Buffer/BufferCounters.h>
#include <Logme/CritSection.h>
#include <Logme/Buffer/DataBuffer.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

namespace Logme
{
  class BufferQueue
  {
  public:
    enum class SoftFlushState
    {
      EMPTY,
      MARKED,
      PUBLISH,
    };

    struct Options
    {
      std::size_t BufferSize = 64 * 1024;
      std::size_t BaseFreeLimit = 6;
      std::size_t MaxTotalBuffers = 0;
      std::size_t MaxAdaptiveFreeLimit = 0;
    };

    Options OptionsValue;

  private:
    mutable CS CurrentLock;
    mutable CS ReadyLock;
    mutable CS FreeLock;
    mutable CS CreateLock;

    std::deque<DataBufferPtr> FreeList;
    std::deque<DataBufferPtr> ReadyList;

    DataBufferPtr Current;

    std::size_t TotalBuffers;
    std::size_t AdaptiveFreeLimit;

    bool Signaled;

    std::atomic<std::uint64_t> Appends;
    std::atomic<std::uint64_t> AppendBytes;
    std::atomic<std::uint64_t> EnqueuedBuffers;
    std::atomic<std::uint64_t> WrittenBuffers;
    std::atomic<std::uint64_t> WrittenBytes;
    std::atomic<std::uint64_t> AllocatedBuffers;
    std::atomic<std::uint64_t> DeletedBuffers;
    std::atomic<std::uint64_t> DroppedAppends;
    std::atomic<std::uint64_t> DroppedBytes;
    std::atomic<std::uint64_t> WriteErrors;
    std::atomic<std::uint64_t> SignalsSent;

  public:
    explicit BufferQueue(const Options& options);

    bool Append(
      const char* p
      , std::size_t cb
      , bool& needSignal
      , bool& firstData
    );
    bool TakeReady(std::vector<DataBufferPtr>& out);
    void Recycle(std::vector<DataBufferPtr>& buffers);

    SoftFlushState PrepareSoftFlushCurrent(DataBuffer* &expected);
    bool PublishCurrent(bool& needSignal);
    bool PublishCurrentIfMatches(DataBuffer* expected, bool& needSignal);
    void ReportWrite(std::size_t buffers, std::size_t bytes, bool ok);

    bool HasReady() const;
    bool HasCurrentData() const;

    BufferCounters GetCounters() const;

  private:
    DataBufferPtr TryTakeFreeBuffer();
    DataBufferPtr TryCreateBuffer();
    void ReleaseBuffer(DataBufferPtr buffer);
    void EnqueueReady(DataBufferPtr buffer, bool& needSignal);

    void MaybeGrowAdaptive(bool allocatedBecauseNoFree);
    void DecayFreeLocked();

    void CountAppended(std::size_t cb);
    void CountDropped(std::size_t cb);
    void CountAllocated();
    void CountDeleted();
    void CountEnqueued();
    void CountWritten(std::size_t buffers, std::size_t bytes, bool ok);
    void CountSignal();
  };
}
