#pragma once

#include <cstddef>

#include <Logme/Backend/Backend.h>
#include <Logme/MemoryUsageTracker.h>

namespace Logme
{
  class MemoryTrackedBackend : public Backend
  {
    MemoryUsageTracker MemoryTracker;

  public:
    LOGMELNK MemoryTrackedBackend(ChannelPtr owner, const char* type);

    LOGMELNK std::size_t GetMemoryUsage() const override;

  protected:
    LOGMELNK MemoryUsageTracker* GetMemoryUsageTracker();
    LOGMELNK const MemoryUsageTracker* GetMemoryUsageTracker() const;
  };
}
