#include <Logme/Backend/MemoryTrackedBackend.h>

using namespace Logme;

MemoryTrackedBackend::MemoryTrackedBackend(ChannelPtr owner, const char* type)
  : Backend(owner, type)
{
}

std::size_t MemoryTrackedBackend::GetMemoryUsage() const
{
  return MemoryTracker.GetMemoryUsage();
}

MemoryUsageTracker* MemoryTrackedBackend::GetMemoryUsageTracker()
{
  return &MemoryTracker;
}

const MemoryUsageTracker* MemoryTrackedBackend::GetMemoryUsageTracker() const
{
  return &MemoryTracker;
}
