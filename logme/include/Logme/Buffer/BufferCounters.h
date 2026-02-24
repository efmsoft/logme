#pragma once

#include <cstddef>
#include <cstdint>

#ifndef BFW_ENABLE_COUNTERS
#define BFW_ENABLE_COUNTERS 1
#endif

#if BFW_ENABLE_COUNTERS
#define BFW_CNT(x) x
#else
#define BFW_CNT(x) ((void)0)
#endif

namespace Logme
{
  struct BufferCounters
  {
    std::uint64_t Appends = 0;
    std::uint64_t AppendBytes = 0;

    std::uint64_t EnqueuedBuffers = 0;
    std::uint64_t WrittenBuffers = 0;
    std::uint64_t WrittenBytes = 0;

    std::uint64_t AllocatedBuffers = 0;
    std::uint64_t DeletedBuffers = 0;

    std::uint64_t DroppedAppends = 0;
    std::uint64_t DroppedBytes = 0;

    std::uint64_t WriteErrors = 0;

    std::uint64_t SignalsSent = 0;
  };
}