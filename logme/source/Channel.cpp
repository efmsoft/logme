#include <Logme/Channel.h>
#include <Logme/Logger.h>
#include <Logme/SafeID.h>

#include <cassert>
#include <string.h>

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

  IDPtr p = std::make_shared<SafeID>(to);
  Link.swap(p);
}

void Channel::AddLink(ChannelPtr to)
{
  Guard guard(DataLock);

  LinkTo = to;
}

void Channel::RemoveLink()
{
  Guard guard(DataLock);

  Link.reset();
  LinkTo.reset();
}

bool Channel::IsLinked() const
{
  return Link != nullptr || LinkTo != nullptr;
}

ChannelPtr Channel::GetLinkPtr()
{
  Guard guard(DataLock);
  return LinkTo;
}

void Channel::SetDisplayFilter(TDisplayFilter filter)
{
  Guard guard(DataLock);
  DisplayFilter = filter;
}

void Channel::Display(Context& context, const char* line)
{
  DataLock.lock();
  AccessCount++;

  if (DisplayFilter)
  {
    TDisplayFilter f = DisplayFilter;
    DataLock.unlock();

    if (f(context, line) == false)
      return;

    DataLock.lock();
  }

  if (context.ErrorLevel < LevelFilter)
  {
    DataLock.unlock();
    return;
  }

  OutputFlags flags = Flags;
  if (context.Ovr)
  {
    flags.Value |= context.Ovr->Add.Value;
    flags.Value &= ~context.Ovr->Remove.Value;
  }

  if ((Link || LinkTo) && !flags.DisableLink)
  {
    IDPtr link = Link;
    DataLock.unlock();

    // Owner->GetChannel /  ch->Display have to be called w/o acquired lock!!
    ChannelPtr ch = LinkTo ? LinkTo : Owner->GetChannel(*link);

    if (ch)
      ch->Display(context, line);

    DataLock.lock();
  }

  for (auto it = Backends.begin(); it != Backends.end(); ++it)
  {
    auto& p = *it;

    if (p->Freezed == false)
      p->Display(context, line);
  }

  DataLock.unlock();
}

bool Channel::IsIdle()
{
  bool idle = true;
  DataLock.lock();

  for (auto it = Backends.begin(); it != Backends.end(); ++it)
  {
    auto& p = *it;
    if (p->IsIdle() == false)
    {
      idle = false;
      break;
    }
  }

  DataLock.unlock();
  return idle;
}

void Channel::Freeze()
{
  DataLock.lock();

  for (auto it = Backends.begin(); it != Backends.end(); ++it)
  {
    auto& p = *it;
    
    p->Freeze();
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
  Guard guard(DataLock);
  Backends.clear();
}

bool Channel::RemoveBackend(BackendPtr backend)
{
  Guard guard(DataLock);
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

  Guard guard(DataLock);
  for (auto it = Backends.begin(); it != Backends.end(); ++it)
  {
    auto& p = *it;
    if (p.get() == backend.get())
      return;
  }

  Backends.push_back(backend);
}

BackendPtr Channel::GetBackend(size_t index)
{
  Guard guard(DataLock);

  if (index < Backends.size())
    return Backends[index];

  return BackendPtr();
}

size_t Channel::NumberOfBackends()
{
  Guard guard(DataLock);
  return Backends.size();
}

BackendPtr Channel::FindFirstBackend(const char* type, int& context)
{
  context = -1;
  return FindNextBackend(type, context);
}

BackendPtr Channel::FindNextBackend(const char* type, int& context)
{
  assert(type);
  std::string stype(type);

  Guard guard(DataLock);
  size_t pos = size_t(context) + 1;
  for (; pos < Backends.size(); pos++)
  {
    auto& b = Backends[pos];
    if (b->GetType() == stype)
    {
      context = int(pos);
      return b;
    }
  }
  return BackendPtr();
}

std::string Channel::GetName()
{
  Guard guard(DataLock);
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

void Channel::ShortenerAdd(const char* what, const char* replace_on)
{
  auto it = ShortenerMap.find(what);
  if (it != ShortenerMap.end())
    it->second = replace_on ? replace_on : "";
  else
  {
    ShortenerMap[what] = replace_on;
  }
}

const char* Channel::ShortenerRun(
  const char* value
  , ShortenerContext& context
)
{
  auto n = strlen(value);
  for (auto& v : ShortenerMap)
  {
    if (n < v.first.length())
      continue;

    if (strncmp(value, v.first.c_str(), v.first.length()))
      continue;

    if (v.second.empty())
      return value + v.first.length();

    size_t cb = v.second.length() + n - v.first.length();
    if (cb < sizeof(context.StaticBuffer))
    {
      strcpy(context.StaticBuffer, v.second.c_str());
      strcpy(context.StaticBuffer + v.second.size(), value + v.first.length());
      return context.StaticBuffer;
    }

    context.Buffer = v.second + (value + v.first.length());
    return context.Buffer.c_str();
  }

  return value;
}

void Channel::SetThreadName(uint64_t id, const char* name)
{
  auto it = ThreadName.find(id);
  if (it != ThreadName.end())
  {
    if (name)
      it->second = name;
    else
      ThreadName.erase(it);
  }
  else
  {
    if (name)
      ThreadName[id] = name;
  }
}

const char* Channel::GetThreadName(uint64_t id)
{
  auto it = ThreadName.find(id);
  if (it != ThreadName.end())
    return it->second.c_str();

  return nullptr;
}
