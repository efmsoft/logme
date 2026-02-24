#pragma once

#include <Logme/Buffer/BufferCounters.h>
#include <Logme/Buffer/DataBuffer.h>

#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

namespace Logme
{
  class BufferQueue
  {
  public:
    struct Options
    {
      std::size_t BufferSize = 64 * 1024;
      std::size_t BaseFreeLimit = 6;
      std::size_t MaxTotalBuffers = 0;
      std::size_t MaxAdaptiveFreeLimit = 0;
    };

    Options OptionsValue;

    mutable std::mutex Lock;

    std::deque<DataBufferPtr> FreeList;
    std::deque<DataBufferPtr> ReadyList;

    DataBufferPtr Current;

    std::size_t TotalBuffers;
    std::size_t AdaptiveFreeLimit;

    bool Signaled;

    BufferCounters Counters;

  public:
    explicit BufferQueue(const Options& options);

    bool Append(const char* p, std::size_t cb, bool& needSignal);
    bool SwapReady(std::vector<DataBufferPtr>& out);
    void Recycle(std::vector<DataBufferPtr>& buffers);
    bool FlushCurrent(bool& needSignal);
    void ReportWrite(std::size_t buffers, std::size_t bytes, bool ok);

    BufferCounters GetCounters() const;

    std::size_t GetTotalBuffers() const;
    std::size_t GetAdaptiveFreeLimit() const;

  private:
    DataBufferPtr AcquireBufferLocked();
    void ReleaseBufferLocked(DataBufferPtr buffer);
    void EnqueueReadyLocked(DataBufferPtr buffer, bool& needSignal);

    void MaybeGrowAdaptiveLocked(bool allocatedBecauseNoFree);
    void DecayLocked();
  };
}