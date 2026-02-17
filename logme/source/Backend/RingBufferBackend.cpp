#include <algorithm>
#include <cstring>
#include <iterator>

#include <Logme/Backend/RingBufferBackend.h>
#include <Logme/Channel.h>

using namespace Logme;

RingBufferBackend::RingBufferBackend(Logme::ChannelPtr owner)
  : Backend(owner, TYPE_ID)
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

  while (Ring.size() > Config.MaxItems)
    Ring.pop_front();
}

void RingBufferBackend::Display(Logme::Context& context, const char* line)
{
  OutputFlags flags = Owner->GetFlags();

  int nc;
  const char* str = context.Apply(Owner, flags, line, nc);

  Append(str, nc);
}

