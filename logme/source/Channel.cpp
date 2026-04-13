#include <Logme/Channel.h>
#include <Logme/Logme.h>
#include <Logme/ReentryGuard.h>
#include <Logme/SafeID.h>

#include <cassert>
#include <cstdio>
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
  , ChannelID(ID{ Name.c_str()})
  , Flags(flags)
  , LevelFilter(level)
  , Enabled(true)
  , Linked(false)
  , BackendCount(0)
  , Active(false)
  , AccessCount(0)
  , ShortenerList(nullptr)
{
}

Channel::~Channel()
{
  Backends.clear();
}

const ID& Channel::GetID() const
{
  return ChannelID;
}

CS& Channel::GetDataLock()
{
  return DataLock;
}

void Channel::UpdateActive()
{
  bool linked = Linked.load(std::memory_order_relaxed);
  bool enabled = Enabled.load(std::memory_order_relaxed);
  size_t backendCount = BackendCount.load(std::memory_order_relaxed);

  Active.store(linked || (enabled && backendCount > 0), std::memory_order_relaxed);
}

bool Channel::operator==(const char* name) const
{
  assert(name);
  return Name == name;
}

void Channel::AddLink(const ID& to)
{
  std::lock_guard guard(DataLock);

  IDPtr p = std::make_shared<SafeID>(to);
  Link.swap(p);

  Linked.store(true, std::memory_order_relaxed);
  UpdateActive();
}

void Channel::AddLink(const ChannelPtr& to)
{
  if (ShutdownCalled)
    return;

  std::lock_guard guard(DataLock);

  LinkTo = to;

  Linked.store(true, std::memory_order_relaxed);
  UpdateActive();
}

void Channel::RemoveLink()
{
  std::lock_guard guard(DataLock);

  Link.reset();
  LinkTo.reset();

  Linked.store(false, std::memory_order_relaxed);
  UpdateActive();
}

bool Channel::IsLinked() const
{
  return Linked.load(std::memory_order_relaxed);
}

bool Channel::IsOutputActive(const Context& context) const
{
  if (context.ErrorLevel < LevelFilter.load(std::memory_order_relaxed))
    return false;

  return Active.load(std::memory_order_relaxed);
}

ChannelPtr Channel::GetLinkPtr()
{
  std::lock_guard guard(DataLock);
  return LinkTo;
}

void Channel::SetDisplayFilter(TDisplayFilter filter)
{
  std::lock_guard guard(DataLock);
  DisplayFilter = filter;
}

void Channel::Display(Context& context, const char* line)
{
  if (ShutdownCalled)
    return;

  AccessCount.fetch_add(1, std::memory_order_relaxed);

  DataLock.lock();

  if (Enabled.load(std::memory_order_relaxed) == false)
  {
    DataLock.unlock();
    return;
  }

  DisplayReentryGuard guard(this);
  if (guard.IsActive() == false)
  {
    DataLock.unlock();
    return;
  }

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
    ChannelPtr pch = LinkTo;

    // Owner->GetChannel / ch->Display have to be called w/o acquired lock!!
    DataLock.unlock();
    ChannelPtr ch = pch ? pch : Owner->GetChannel(*link);

    if (ch)
    {
      // We have to apply context right now to use this channel settings

      int nc = 0;
      context.Apply(shared_from_this(), flags, line, nc);

      ch->Display(context, line);
    }

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

void Channel::Flush()
{
  std::lock_guard guard(DataLock);

  for (auto& b : Backends)
    b->Flush();
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
  Freeze();

  std::lock_guard guard(DataLock);
  Backends.clear();

  BackendCount.store(0, std::memory_order_relaxed);
  UpdateActive();
}

bool Channel::RemoveBackend(BackendPtr backend)
{
  std::lock_guard guard(DataLock);
  for (auto it = Backends.begin(); it != Backends.end(); ++it)
  {
    auto& p = *it;
    if (p.get() == backend.get())
    {
      p->Freeze();
     
      Backends.erase(it);

      BackendCount.store(Backends.size(), std::memory_order_relaxed);
      UpdateActive();
      return true;
    }
  }
  return false;
}

void Channel::AddBackend(BackendPtr backend)
{
  assert(backend);

  if (ShutdownCalled)
    return;

  std::lock_guard guard(DataLock);
  for (auto it = Backends.begin(); it != Backends.end(); ++it)
  {
    auto& p = *it;
    if (p.get() == backend.get())
      return;
  }

  Backends.push_back(backend);

  BackendCount.store(Backends.size(), std::memory_order_relaxed);
  UpdateActive();
}

BackendPtr Channel::GetBackend(size_t index)
{
  std::lock_guard guard(DataLock);

  if (index < Backends.size())
    return Backends[index];

  return BackendPtr();
}

size_t Channel::NumberOfBackends() const
{
  return BackendCount.load(std::memory_order_relaxed);
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

  std::lock_guard guard(DataLock);
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
  std::lock_guard guard(DataLock);
  return Name;
}

Level Channel::GetFilterLevel() const
{
  return LevelFilter.load(std::memory_order_relaxed);
}

void Channel::SetFilterLevel(Level level)
{
  LevelFilter.store(level, std::memory_order_relaxed);
}

void Channel::SetEnabled(bool enable)
{
  Enabled.store(enable, std::memory_order_relaxed);
  UpdateActive();
}

bool Channel::GetEnabled() const
{
  return Enabled.load(std::memory_order_relaxed);
}

bool Channel::GetActive() const
{
  return Active.load(std::memory_order_relaxed);
}

uint64_t Channel::GetAccessCount() const
{
  return AccessCount.load(std::memory_order_relaxed);
}

Logger* Channel::GetOwner() const
{
  return Owner;
}

void Channel::SetShortenerPair(const ShortenerPair* pair)
{
  std::lock_guard guard(ShortenerLock);
  ShortenerList = pair;
}

void Channel::ShortenerAdd(const char* what, const char* replace_on)
{
  std::lock_guard guard(ShortenerLock);

  auto it = ShortenerMap.find(what);
  if (it != ShortenerMap.end())
  {
    if (replace_on)
      it->second = replace_on;
    else
      ShortenerMap.erase(it);
  }
  else if (replace_on)
  {
    ShortenerMap[what] = replace_on;
  }
}

const char* Channel::ShortenerPairRun(
  const char* value
  , ShortenerContext& context
  , const ShortenerPair* pair
)
{
  if (value == context.StaticBuffer)
    return value;

  if (context.Buffer && value == context.Buffer->c_str())
    return value;

  auto n = strlen(value);
  for (; pair->SerachFor && pair->ReplaceOn; pair++)
  {
    auto ls = strlen(pair->SerachFor);

    if (n < ls)
      continue;

    if (strncmp(value, pair->SerachFor, ls))
      continue;

    auto lr = strlen(pair->ReplaceOn);

    if (lr == 0)
      return value + ls;

    size_t cb = lr + n - ls;
    if (cb < sizeof(context.StaticBuffer))
    {
      strcpy(context.StaticBuffer, pair->ReplaceOn);
      strcpy(context.StaticBuffer + lr, value + ls);
      return context.StaticBuffer;
    }

    if (context.Buffer)
      context.Buffer->assign(std::string(pair->ReplaceOn) + (value + ls));
    else
      context.Buffer = std::make_unique<std::string>(std::string(pair->ReplaceOn) + (value + ls));

    return context.Buffer->c_str();
  }

  return value;
}

const char* Channel::ShortenerRun(
  const char* value
  , ShortenerContext& context
  , Override& ovr
)
{
  if (ovr.Shortener == nullptr)
    return value;

  return ShortenerPairRun(value, context, ovr.Shortener);
}

const char* Channel::ShortenerRun(
  const char* value
  , ShortenerContext& context
)
{
  std::lock_guard guard(ShortenerLock);

  if (ShortenerList)
    value = ShortenerPairRun(value, context, ShortenerList);

  if (value == context.StaticBuffer)
    return value;

  if (context.Buffer && value == context.Buffer->c_str())
    return value;
  
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

    if (context.Buffer)
      context.Buffer->assign(v.second + (value + v.first.length()));
    else
      context.Buffer = std::make_unique<std::string>(v.second + (value + v.first.length()));
    
    return context.Buffer->c_str();
  }

  return value;
}

void Channel::SetThreadName(uint64_t id, const char* name, bool log)
{
  std::lock_guard guard(DataLock);

  auto it = ThreadName.find(id);
  if (it != ThreadName.end())
  {
    if (log)
      it->second.Prev = it->second.Name;
    else
      it->second.Prev.reset();

    if (name)
      it->second.Name = name;
    else if (log == false)
      ThreadName.erase(it);
    else 
      it->second.Name.reset();
  }
  else
  {
    if (name)
    {
      ThreadNameRecord r;
      r.Name = name;

      if (log)
      {
        char buf[32];
        snprintf(buf, sizeof(buf), LOGME_FMT_U64_HEX_UPPER, (uint64_t)id);
        r.Prev = buf;
      }

      ThreadName[id] = r;
    }
  }
}

const char* Channel::GetThreadName(
  uint64_t id
  , ThreadNameInfo& info
  , std::optional<std::string>* transition
  , bool clear
)
{
  if (transition)
    transition->reset();

  std::lock_guard guard(DataLock);

  auto it = ThreadName.find(id);
  if (it != ThreadName.end())
  {
    if (it->second.Name.has_value() == false && it->second.Prev.has_value() == false)
    {
      ThreadName.erase(it);
      return nullptr;
    }

    if (transition)
      *transition = it->second.Prev;

    if (clear)
      it->second.Prev.reset();

    if (it->second.Name.has_value())
    {
      const std::string& s = it->second.Name.value();
      if (s.size()  < sizeof(info.Buffer))
      {
        strcpy(info.Buffer, s.c_str());
        return info.Buffer;
      }

      info.Name = s;
      return info.Name.c_str();
    }

    return nullptr;
  }

  return nullptr;
}
