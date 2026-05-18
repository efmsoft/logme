#include <Logme/MemoryUsageTracker.h>

using namespace Logme;

MemoryUsageTracker::MemoryUsageTracker()
  : MemoryUsage(0)
{
}

void MemoryUsageTracker::AddMemoryUsage(std::size_t bytes)
{
  MemoryUsage.fetch_add(bytes, std::memory_order_relaxed);
}

void MemoryUsageTracker::RemoveMemoryUsage(std::size_t bytes)
{
  MemoryUsage.fetch_sub(bytes, std::memory_order_relaxed);
}

std::size_t MemoryUsageTracker::GetMemoryUsage() const
{
  return MemoryUsage.load(std::memory_order_relaxed);
}
