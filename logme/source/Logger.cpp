#include <Logme/Logger.h>
#include <Logme/Backend/ConsoleBackend.h>
#include <Logme/Backend/DebugBackend.h>
#include <Logme/File/exe_path.h>
#include <Logme/Time/datetime.h>
#include <Logme/Utils.h>

#include "StringHelpers.h"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <functional>
#include <stdarg.h>

#pragma warning(disable : 6255)

using namespace Logme;

LoggerPtr Logme::Instance = std::make_shared<Logger>();

Logger::Logger()
  : HomeDirectoryWatchDog(HomeDirectory, std::bind_front(&Logger::TestFileInUse, this))
  , BlockReportedSubsystems(true)
  , IDGenerator(1)
  , ControlSocket(-1)
  , LastDoAutodelete(0)
  , NumDeleting(0)
  , Condition(&Logger::DefaultCondition)
{
  CreateDefaultChannelLayout();

  HomeDirectory = GetExecutablePath();
}

Logger::~Logger()
{
  Factory.SetStopping();
  StopControlServer();
  DeleteAllChannels();
}

void Logger::Shutdown()
{
  if (Instance)
  {
    Instance->StopControlServer();
    Instance->DeleteAllChannels();
  }
}

void Logger::SetCondition(TCondition cond)
{
  Condition = cond == nullptr ? &Logger::DefaultCondition : cond;
}

bool Logger::DefaultCondition()
{
  return true;
}

void Logger::CreateDefaultChannelLayout(bool delete_all)
{
  if (delete_all)
    DeleteAllChannels();

  if (Default == nullptr)
  {
    Default = std::make_shared<Channel>(this, nullptr, OutputFlags(), DEFAULT_LEVEL);

    Default->AddBackend(std::make_shared<ConsoleBackend>(Default));
#ifdef _DEBUG
    Default->AddBackend(std::make_shared<DebugBackend>(Default));
#endif
  }
}

void Logger::DeleteAllChannels()
{
  // Backends keep shared_ptr to owner. We have to clear it to delete channel!
  if (Default)
    Default->RemoveBackends();

  DoAutodelete(true);

  for (auto& c : Channels)
  {
    c.second->RemoveLink();
    c.second->RemoveBackends();
  }

  Channels.clear();
  Default.reset();
}

void Logger::SetHomeDirectory(const std::string& path)
{
  HomeDirectory = path;

  if (HomeDirectory.empty() == false)
  {
    if (!HomeDirectory.ends_with(std::filesystem::path::preferred_separator))
      HomeDirectory += std::filesystem::path::preferred_separator;
  }
}

const std::string& Logger::GetHomeDirectory() const
{
  return HomeDirectory;
}

bool Logger::TestFileInUse(const std::string& file)
{
  return Factory.TestFileInUse(file);
}

FileManagerFactory& Logger::GetFileManagerFactory()
{
  return Factory;
}

StringPtr Logger::GetErrorChannel()
{
  std::lock_guard guard(ErrorLock);
  return ErrorChannel;
}

void Logger::SetErrorChannel(const char* name)
{
  std::lock_guard guard(ErrorLock);

  if (ErrorChannel == nullptr)
    ErrorChannel = std::make_shared<std::string>(name);
  else
    *ErrorChannel = name;
}

void Logger::SetErrorChannel(const std::string& name)
{
  SetErrorChannel(name.c_str());
}

void Logger::SetErrorChannel(const ID& ch)
{
  SetErrorChannel(ch.Name);
}

void Logger::SetThreadOverride(const Override* ovr)
{
  uint64_t tid = GetCurrentThreadId();

  std::lock_guard guard(DataLock);

  if (ovr == nullptr)
  {
    auto it = ThreadOverride.find(tid);
    if (it != ThreadOverride.end())
      ThreadOverride.erase(it);
  }
  else
    ThreadOverride[tid] = *ovr;
}

Override Logger::GetThreadOverride()
{
  uint64_t tid = GetCurrentThreadId();

  std::lock_guard guard(DataLock);

  auto it = ThreadOverride.find(tid);
  if (it != ThreadOverride.end())
    return it->second;

  return Override();
}

void Logger::SetThreadChannel(const ID* id)
{
  uint64_t tid = GetCurrentThreadId();
  
  std::lock_guard guard(DataLock);

  if (id == nullptr)
  {
    auto it = ThreadChannel.find(tid);
    if (it != ThreadChannel.end())
      ThreadChannel.erase(it);
  }
  else
    ThreadChannel[tid] = *id;
}

bool Logger::IsChannelDefinedForCurrentThread()
{
  uint64_t tid = GetCurrentThreadId();

  std::lock_guard guard(DataLock);

  auto it = ThreadChannel.find(tid);
  return it != ThreadChannel.end();
}

void Logger::ApplyThreadChannel(Context& context)
{
  uint64_t tid = GetCurrentThreadId();
  
  std::lock_guard guard(DataLock);

  auto it = ThreadChannel.find(tid);
  if (it != ThreadChannel.end())
  {
    context.ChannelStg = it->second;
    context.Channel = &context.ChannelStg;
  }
}

ID Logger::GetDefaultChannel()
{
  uint64_t tid = GetCurrentThreadId();

  std::lock_guard guard(DataLock);

  auto it = ThreadChannel.find(tid);
  if (it != ThreadChannel.end())
    return it->second;

  return ::CH;
}

ChannelPtr Logger::GetExistingChannel(const ID& id)
{
  if (id.Name == nullptr || *id.Name == '\0')
    return Default;

  std::lock_guard guard(DataLock);

  auto it = Channels.find(id.Name);
  if (it != Channels.end())
    return it->second;

  return ChannelPtr();
}

ChannelPtr Logger::GetChannel(const ID& id)
{
  ChannelPtr v = GetExistingChannel(id);
  if (v)
    return v;

  std::lock_guard guard(DataLock);
  return CreateChannelInternal(id, OutputFlags());
}

void Logger::DoAutodelete(bool force)
{
  if (NumDeleting == 0 && force == false)
    return;

  static const unsigned periodicity = 100;
  
  auto n = GetTimeInMillisec();
  if (n - LastDoAutodelete < periodicity)
    return;

  LastDoAutodelete = GetTimeInMillisec();

  ChannelPtr ch;
  for (bool cont = true; cont;)
  {
    cont = false;

    if (ch)
    {
      ch->RemoveBackends();
      ch->RemoveLink();
      
      ch.reset();
    }

    std::lock_guard guard(DataLock);
    for (auto it = ToDelete.begin(); it != ToDelete.end(); ++it)
    {
      auto& p = *it;
      if (force || p->IsIdle())
      {
        ch = p;
        ToDelete.erase(it);
        NumDeleting--;

        cont = true;
        break;
      }
    }
  }
}

void Logger::Autodelete(const ID& id)
{
  std::lock_guard guard(DataLock);

  auto it = Channels.find(id.Name);
  if (it != Channels.end())
  {
    ChannelPtr ch = it->second;
    Channels.erase(it);

    ch->Freeze();
    ToDelete.push_back(ch);
    NumDeleting++;
  }
}

void Logger::DeleteChannel(const ID& id)
{
  ChannelPtr ch;

  do
  {
    std::lock_guard guard(DataLock);

    auto it = Channels.find(id.Name);
    if (it != Channels.end())
    {
      ch = it->second;
      Channels.erase(it);
    }

  } while (false);

  // Do it without acquired mutex!

  if (ch)
  {
    ch->RemoveBackends();
    ch->RemoveLink();
  }
}

ChannelPtr Logger::CreateChannel(
  const OutputFlags& flags
  , Level level
)
{
  ID id{};
  std::string name;

  std::lock_guard guard(DataLock);

  for (;;)
  {
    name = "#" + std::to_string(++IDGenerator);
    id.Name = name.c_str();

    auto it = Channels.find(id.Name);
    if (it == Channels.end())
      return CreateChannelInternal(id, flags, level);
  }
}

ChannelPtr Logger::CreateChannel(
  const ID& id
  , const OutputFlags& flags
  , Level level
)
{
  std::lock_guard guard(DataLock);

  auto it = Channels.find(id.Name);
  if (it != Channels.end())
    return it->second;

  return CreateChannelInternal(id, flags, level);
}

ChannelPtr Logger::CreateChannelInternal(
  const ID& id
  , const OutputFlags& flags
  , Level level
)
{
  auto it = Channels.find(id.Name);
  if (it != Channels.end())
    return it->second;

  ChannelPtr channel = std::make_shared<Channel>(this, id.Name, flags, level);
  Channels[id.Name] = channel;

  return channel;
}

void Logger::SetBlockReportedSubsystems(bool block)
{
  BlockReportedSubsystems = block;
}

void Logger::ReportSubsystem(const SID& sid)
{
  if (sid.Name == 0)
    return;

  auto& arr = Subsystems;
  auto& id = sid.Name;

  std::lock_guard guard(DataLock);

  auto it = std::lower_bound(arr.begin(), arr.end(), id);
  if (it == arr.end() || *it != id)
    arr.insert(it, id);
}

void Logger::UnreportSubsystem(const SID& sid)
{
  if (sid.Name == 0)
    return;

  auto& arr = Subsystems;
  auto& id = sid.Name;

  std::lock_guard guard(DataLock);

  auto it = std::lower_bound(arr.begin(), arr.end(), id);
  if (it != arr.end() && *it == id)
    arr.erase(it);
}

Stream Logger::Log(const Context& context) // @1
{
  Context& context2 = *(Context*)&context;

  OverridePtr ovr = std::make_shared<Override>(GetThreadOverride());
  context2.Ovr = ovr.get();

  ApplyThreadChannel(context2);

  return Stream(shared_from_this(), context2, ovr);
}

Stream Logger::Log(const Context& context, Override& ovr) // @2
{
  Context& context2 = *(Context*)&context;
  context2.Ovr = &ovr;

  ApplyThreadChannel(context2);

  return Stream(shared_from_this(), context2);
}

Stream Logger::Log(const Context& context, const SID& sid, Override& ovr) // @3
{
  Context& context2 = *(Context*)&context;
  context2.Ovr = &ovr;
  context2.Subsystem = sid;

  ApplyThreadChannel(context2);

  return Stream(shared_from_this(), context2);
}

Stream Logger::Log(const Context& context, const ID& id) // @4
{
  Context& context2 = *(Context*)&context;
  context2.Channel = &id;

  OverridePtr ovr = std::make_shared<Override>(GetThreadOverride());
  context2.Ovr = ovr.get();

  return Stream(shared_from_this(), context2, ovr);
}

Stream Logger::Log(const Context& context, ChannelPtr ch) // @5
{
  Context& context2 = *(Context*)&context;
  context2.Ch = ch;

  OverridePtr ovr = std::make_shared<Override>(GetThreadOverride());
  context2.Ovr = ovr.get();

  return Stream(shared_from_this(), context2, ovr);
}

Stream Logger::Log(const Context& context, const ID& id, const SID& sid) // @6
{
  Context& context2 = *(Context*)&context;
  context2.Channel = &id;
  context2.Subsystem = sid;

  OverridePtr ovr = std::make_shared<Override>(GetThreadOverride());
  context2.Ovr = ovr.get();

  return Stream(shared_from_this(), context2, ovr);
}

Stream Logger::Log(const Context& context, ChannelPtr ch, const SID& sid) // @7
{
  Context& context2 = *(Context*)&context;
  context2.Ch = ch;
  context2.Subsystem = sid;

  OverridePtr ovr = std::make_shared<Override>(GetThreadOverride());
  context2.Ovr = ovr.get();

  return Stream(shared_from_this(), context2, ovr);
}

Stream Logger::Log(const Context& context, const ID& id, Override& ovr) // @8
{
  Context& context2 = *(Context*)&context;
  context2.Channel = &id;
  context2.Ovr = &ovr;

  return Stream(shared_from_this(), context2);
}

Stream Logger::Log(const Context& context, ChannelPtr ch, Override& ovr) // @9
{
  Context& context2 = *(Context*)&context;
  context2.Ch = ch;
  context2.Ovr = &ovr;

  return Stream(shared_from_this(), context2);
}

Stream Logger::Log(const Context& context, const ID& id, const SID& sid, Override& ovr) // @10
{
  Context& context2 = *(Context*)&context;
  context2.Channel = &id;
  context2.Subsystem = sid;
  context2.Ovr = &ovr;

  return Stream(shared_from_this(), context2);
}

Stream Logger::Log(const Context& context, ChannelPtr ch, const SID& sid, Override& ovr) // @11
{
  Context& context2 = *(Context*)&context;
  context2.Ch = ch;
  context2.Subsystem = sid;
  context2.Ovr = &ovr;

  return Stream(shared_from_this(), context2);
}

void Logger::Log(
  const Context& context
  , const ID& id
  , const char* format
  , ...
)
{
  Context& context2 = *(Context*)&context;
  context2.Channel = &id;

  auto ovr = GetThreadOverride();
  context2.Ovr = &ovr;

  va_list args;
  va_start(args, format);

  DoLog(context2, format, args);

  va_end(args);
}

void Logger::Log(
  const Context& context
  , ChannelPtr ch
  , const char* format
  , ...
)
{
  if (ch && (context.ErrorLevel < ch->GetFilterLevel() || ch->GetEnabled() == false))
    return;

  Context& context2 = *(Context*)&context;
  context2.Ch = ch;

  auto ovr = GetThreadOverride();
  context2.Ovr = &ovr;

  va_list args;
  va_start(args, format);

  DoLog(context2, format, args);

  va_end(args);
}

void Logger::Log(const Context& context, const ID& id, const SID& sid, const char* format, ...)
{
  Context& context2 = *(Context*)&context;
  context2.Channel = &id;
  context2.Subsystem = sid;

  auto ovr = GetThreadOverride();
  context2.Ovr = &ovr;

  va_list args;
  va_start(args, format);

  DoLog(context2, format, args);

  va_end(args);
}

void Logger::Log(const Context& context, ChannelPtr ch, const SID& sid, const char* format, ...)
{
  if (ch && (context.ErrorLevel < ch->GetFilterLevel() || ch->GetEnabled() == false))
    return;

  Context& context2 = *(Context*)&context;
  context2.Ch = ch;
  context2.Subsystem = sid;

  auto ovr = GetThreadOverride();
  context2.Ovr = &ovr;

  va_list args;
  va_start(args, format);

  DoLog(context2, format, args);

  va_end(args);
}

void Logger::Log(
  const Context& context
  , const ID& id
  , Override& ovr
  , const char* format
  , ...
)
{
  Context& context2 = *(Context*)&context;
  context2.Channel = &id;
  context2.Ovr = &ovr;

  va_list args;
  va_start(args, format);

  DoLog(context2, format, args);

  va_end(args);
}

void Logger::Log(
  const Context& context
  , ChannelPtr ch
  , Override& ovr
  , const char* format
  , ...
)
{
  if (ch && (context.ErrorLevel < ch->GetFilterLevel() || ch->GetEnabled() == false))
    return;

  Context& context2 = *(Context*)&context;
  context2.Ch = ch;
  context2.Ovr = &ovr;

  va_list args;
  va_start(args, format);

  DoLog(context2, format, args);

  va_end(args);
}

void Logger::Log(
  const Context& context
  , Override& ovr
  , const char* format
  , ...
)
{
  Context& context2 = *(Context*)&context;
  context2.Ovr = &ovr;
  ApplyThreadChannel(context2);

  va_list args;
  va_start(args, format);

  DoLog(context2, format, args);

  va_end(args);
}

void Logger::Log(const Context& context, const char* format, ...)
{
  Context& context2 = *(Context*)&context;

  auto ovr = GetThreadOverride();
  context2.Ovr = &ovr;

  ApplyThreadChannel(context2);

  va_list args;
  va_start(args, format);

  DoLog(context2, format, args);

  va_end(args);
}

void Logger::DoLog(Context& context, const char* format, va_list args)
{
  DoAutodelete(false);

  if (context.Ovr && context.Ovr->MaxFrequency)
  {
    auto ticks = GetTimeInMillisec();

    std::lock_guard guard(DataLock);
    if (context.Ovr->LastTime && ticks - context.Ovr->LastTime < context.Ovr->MaxFrequency)
      return;

    context.Ovr->LastTime = ticks;
  }

  if (context.Ovr && context.Ovr->MaxRepetitions != -1)
  {
    std::lock_guard guard(DataLock);
    if (context.Ovr->Repetitions >= context.Ovr->MaxRepetitions)
      return;

    context.Ovr->Repetitions++;
  }

  if (context.Subsystem.Name)
  {
    auto& arr = Subsystems;
    if (std::binary_search(arr.begin(), arr.end(), context.Subsystem.Name))
    {
      if (BlockReportedSubsystems)
        return;
    }
    else if (BlockReportedSubsystems == false)
      return;
  }

  ChannelPtr ch = context.Ch ? context.Ch : GetChannel(*context.Channel);  
  if (ch == nullptr)
    return;

  if (context.ErrorLevel < ch->GetFilterLevel() && !ch->IsLinked())
  {
    if (context.ErrorLevel < Level::LEVEL_ERROR)
      return;

    StringPtr errorChannel = GetErrorChannel();
    if (errorChannel == nullptr || ch->GetName() == *errorChannel)
      return;
  }

  if (format)
  {
    char* buffer = nullptr;
    
    if (strcmp(format, "%s") == 0)
    { 
      buffer = va_arg(args, char*);
    }
    else
    {
      size_t size = std::max(16384U, (unsigned int)(strlen(format) + 512));

      buffer = (char*)alloca(size);

      buffer[0] = '\0';
      buffer[size - 1] = '\0';

      int rc = vsnprintf(buffer + strlen(buffer), size - 1, format, args);
      if (rc == -1)
        strcpy_s(buffer, size - 1, "[format error]");
    }

    ch->Display(context, buffer);

    if (context.ErrorLevel >= Level::LEVEL_ERROR)
    {
      StringPtr errorChannel = GetErrorChannel();
      if (errorChannel != nullptr)
      {
        if (ch->GetName() != *errorChannel)
        {
          ID id{ errorChannel->c_str() };
          ChannelPtr chError = GetExistingChannel(id);

          if (chError != nullptr)
          {
            Override ovr;
            if (context.Ovr)
              ovr = *context.Ovr;

            ovr.Add.DisableLink = true;

            Override* ovrbk = context.Ovr;
            context.Ovr = &ovr;

            chError->Display(context, buffer);
            
            context.Ovr = ovrbk;
          }
        }
      }
    }

    if (context.Output)
      *context.Output = buffer;
  }
}

void Logger::IterateChannels(const TChannelCallback& callback)
{
  std::lock_guard guard(DataLock);

  if (Default)
    callback("", Default);

  for (const auto& [name, channel] : Channels)
    callback(name, channel);
}
