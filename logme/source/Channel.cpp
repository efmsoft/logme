#include <Logme/Channel.h>
#include <Logme/Logger.h>

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

const ID Channel::GetID() const
{
  return ID{ Name.c_str() };
}

bool Channel::operator==(const char* name) const
{
  assert(name);
  return Name == name;
}

void Channel::AddLink(const ID& to)
{
  Guard guard(DataLock);

  IDPtr p = std::make_shared<ID>(to);
  Link.swap(p);
}

void Channel::RemoveLink()
{
  Guard guard(DataLock);
  Link.reset();
}

bool Channel::IsLinked() const
{
  return Link != nullptr;
}

void Channel::Display(Context& context, const char* line)
{
  DataLock.lock();
  AccessCount++;

  if (context.ErrorLevel < LevelFilter)
  {
    DataLock.unlock();
    return;
  }

  OutputFlags flags = Flags;
  flags.Value |= context.Ovr.Add.Value;
  flags.Value &= ~context.Ovr.Remove.Value;

  if (Link && !flags.DisableLink)
  {
    IDPtr link = Link;
    DataLock.unlock();

    // Owner->GetChannel /  ch->Display have to be called w/o acquired lock!!
    ChannelPtr ch = Owner->GetChannel(*link);

    if (ch)
      ch->Display(context, line);

    DataLock.lock();
  }

  for (auto it = Backends.begin(); it != Backends.end(); ++it)
  {
    auto& p = *it;
    p->Display(context, line);
  }

  DataLock.unlock();
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

bool Channel::RemoveBackend(BackendPtr backend)
{
  Guard guars(DataLock);
  for (auto it = Backends.begin(); it != Backends.end(); ++it)
  {
    auto& p = *it;
    if (p.get() == backend.get())
    {
      Backends.erase(it);
      return true;
    }
  }
  return false;
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
