#pragma once

#include <cstddef>
#include <cstdint>

#ifndef FILE_ENABLE_COUNTERS
#define FILE_ENABLE_COUNTERS 0
#endif

#if FILE_ENABLE_COUNTERS
#define FILE_CNT(x) x
#else
#define FILE_CNT(x) ((void)0)
#endif

#define BFW_CNT(x) FILE_CNT(x)

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