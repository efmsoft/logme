#include <algorithm>
#include <cstring>
#include <iterator>
#include <sstream>

#include <Logme/Backend/RingBufferBackend.h>
#include <Logme/Channel.h>
#include <Logme/MemoryUsageTracker.h>

using namespace Logme;

RingBufferBackend::RingBufferBackend(Logme::ChannelPtr owner)
  : MemoryTrackedBackend(owner, TYPE_ID)
{
}

BackendConfigPtr RingBufferBackend::CreateConfig()
{
  return std::make_shared<RingBufferBackendConfig>();
}

bool RingBufferBackend::ApplyConfig(BackendConfigPtr c)
{
  if (c == nullptr || c->Type != TYPE_ID)
    return false;

  RingBufferBackendConfig* p = (RingBufferBackendConfig*)c.get();
  
  std::lock_guard guard(Lock);
  Config = *p;

  return true;
}

void RingBufferBackend::Clear()
{
  std::lock_guard guard(Lock);

  for (const auto& item : Ring)
    GetMemoryUsageTracker()->RemoveMemoryUsage(item.capacity());

  Ring.clear();
}

std::string RingBufferBackend::Join()
{
  std::lock_guard guard(Lock);

  size_t size = 1;
  for (const auto& s : Ring)
    size += s.size();

  std::string result;
  result.reserve(size);

  for (const auto& s : Ring)
    result.append(s);

  return result;
}

void RingBufferBackend::Append(const char* str, int nc)
{
  if (nc == -1)
    nc = (int)strlen(str);

  if (nc == 0)
    return;

  std::lock_guard guard(Lock);

  Ring.emplace_back(str, nc);
  GetMemoryUsageTracker()->AddMemoryUsage(Ring.back().capacity());

  while (Ring.size() > Config.MaxItems)
  {
    GetMemoryUsageTracker()->RemoveMemoryUsage(Ring.front().capacity());
    Ring.pop_front();
  }
}


std::string RingBufferBackend::FormatDetails()
{
  std::lock_guard guard(Lock);

  std::ostringstream os;
  os << "MaxItems=" << Config.MaxItems;
  os << " Used=" << Ring.size();

  size_t memoryUsage = GetMemoryUsage();
  if (memoryUsage != 0)
    os << " Memory=" << memoryUsage;

  return os.str();
}

void RingBufferBackend::Display(Logme::Context& context)
{
  OutputFlags flags = Owner->GetFlags();

  int nc;
  const char* str = context.Apply(Owner, flags, nc);

  Append(str, nc);
}

