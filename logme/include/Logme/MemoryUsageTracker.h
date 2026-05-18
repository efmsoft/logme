#pragma once

#include <atomic>
#include <cstddef>

#include <Logme/Types.h>

namespace Logme
{
  class MemoryUsageTracker
  {
    std::atomic<std::size_t> MemoryUsage;

  public:
    LOGMELNK MemoryUsageTracker();

    LOGMELNK void AddMemoryUsage(std::size_t bytes);
    LOGMELNK void RemoveMemoryUsage(std::size_t bytes);
    LOGMELNK std::size_t GetMemoryUsage() const;
  };
}
