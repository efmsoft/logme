#include <Logme/Channel.h>

#include <cassert>

using namespace Logme;

Channel::Channel(
  Logger* owner
  , const char* name
  , const OutputFlags& flags
  , Level level
)
  : Owner(owner)
  , Name(name ? name : "")
  , Flags(flags)
  , LevelFilter(level)
  , AccessCount(0)
{
}

Channel::~Channel()
{
  Backends.clear();
}

bool Channel::operator==(const char* name) const
{
  assert(name);
  return Name == name;
}

void Channel::Display(Context& context, const char* line)
{
  Guard guars(DataLock);
  AccessCount++;

  if (context.ErrorLevel < LevelFilter)
    return;

  for (auto it = Backends.begin(); it != Backends.end(); ++it)
  {
    auto& p = *it;
    p->Display(context, line);
  }
}

const OutputFlags& Channel::GetFlags() const
{
  return Flags;
}

void Channel::SetFlags(const OutputFlags& flags)
{
  Flags = flags;
}

void Channel::RemoveBackends()
{
  Guard guars(DataLock);
  Backends.clear();
}

void Channel::AddBackend(BackendPtr backend)
{
  assert(backend);

  Guard guars(DataLock);
  for (auto it = Backends.begin(); it != Backends.end(); ++it)
  {
    auto& p = *it;
    if (p.get() == backend.get())
      return;
  }

  Backends.push_back(backend);
}

std::string Channel::GetName()
{
  Guard guars(DataLock);
  return Name;
}

Level Channel::GetFilterLevel() const
{
  return LevelFilter;
}

void Channel::SetFilterLevel(Level level)
{
  LevelFilter = level;
}

uint64_t Channel::GetAccessCount() const
{
  return AccessCount;
}

Logger* Channel::GetOwner() const
{
  return Owner;
}
