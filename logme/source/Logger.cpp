#include <Logme/Logger.h>
#include <Logme/Backend/ConsoleBackend.h>
#include <Logme/Backend/DebugBackend.h>
#include <Logme/File/exe_path.h>
#include <Logme/Time/datetime.h>
#include <Logme/Utils.h>

#include "StringHelpers.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <stdarg.h>

#pragma warning(disable : 6255)

using namespace Logme;

LoggerPtr Logme::Instance = std::make_shared<Logger>();

Logger::Logger()
  : IDGenerator(1)
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

const std::string& Logger::GetHomeDirectory() const
{
  return HomeDirectory;
}

FileManagerFactory& Logger::GetFileManagerFactory()
{
  return Factory;
}

StringPtr Logger::GetErrorChannel()
{
  Guard guard(ErrorLock);
  return ErrorChannel;
}

void Logger::SetErrorChannel(const char* name)
{
  Guard guard(ErrorLock);

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

void Logger::SetThreadChannel(const ID* id)
{
  uint64_t tid = GetCurrentThreadId();
  
  Guard guard(DataLock);

  if (id == nullptr)
  {
    auto it = ThreadChannel.find(tid);
    if (it != ThreadChannel.end())
      ThreadChannel.erase(it);
  }
  else
    ThreadChannel[tid] = *id;
}

void Logger::ApplyThreadChannel(Context& context)
{
  uint64_t tid = GetCurrentThreadId();
  
  Guard guard(DataLock);
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

  Guard guard(DataLock);

  auto it = ThreadChannel.find(tid);
  if (it != ThreadChannel.end())
    return it->second;

  return ::CH;
}

ChannelPtr Logger::GetExistingChannel(const ID& id)
{
  if (id.Name == nullptr || *id.Name == '\0')
    return Default;

  Guard guard(DataLock);

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

  Guard guard(DataLock);
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

    Guard guard(DataLock);
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
  Guard guard(DataLock);

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
    Guard guard(DataLock);

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

  Guard guard(DataLock);

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
  Guard guard(DataLock);

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

Stream Logger::Log(const Context& context)
{
  Context& context2 = *(Context*)&context;
  ApplyThreadChannel(context2);

  return Stream(shared_from_this(), context2);
}

Stream Logger::Log(const Context& context, const ID& id)
{
  Context& context2 = *(Context *)&context;
  context2.Channel = &id;

  return Stream(shared_from_this(), context2);
}

Stream Logger::Log(const Context& context, ChannelPtr ch)
{
  Context& context2 = *(Context*)&context;
  context2.Ch = ch;

  return Stream(shared_from_this(), context2);
}

Stream Logger::Log(const Context& context, const ID& id, const Override& ovr)
{
  Context& context2 = *(Context*)&context;
  context2.Channel = &id;
  context2.Ovr = ovr;

  return Stream(shared_from_this(), context2);
}

Stream Logger::Log(const Context& context, ChannelPtr ch, const Override& ovr)
{
  Context& context2 = *(Context*)&context;
  context2.Ch = ch;
  context2.Ovr = ovr;

  return Stream(shared_from_this(), context2);
}

Stream Logger::Log(const Context& context, const Override& ovr)
{
  Context& context2 = *(Context*)&context;
  context2.Ovr = ovr;

  ApplyThreadChannel(context2);

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
  Context& context2 = *(Context*)&context;
  context2.Ch = ch;

  va_list args;
  va_start(args, format);

  DoLog(context2, format, args);

  va_end(args);
}

void Logger::Log(
  const Context& context
  , const ID& id
  , const Override& ovr
  , const char* format
  , ...
)
{
  Context& context2 = *(Context*)&context;
  context2.Channel = &id;
  context2.Ovr = ovr;

  va_list args;
  va_start(args, format);

  DoLog(context2, format, args);

  va_end(args);
}

void Logger::Log(
  const Context& context
  , ChannelPtr ch
  , const Override& ovr
  , const char* format
  , ...
)
{
  Context& context2 = *(Context*)&context;
  context2.Ch = ch;
  context2.Ovr = ovr;

  va_list args;
  va_start(args, format);

  DoLog(context2, format, args);

  va_end(args);
}

void Logger::Log(
  const Context& context
  , const Override& ovr
  , const char* format
  , ...
)
{
  Context& context2 = *(Context*)&context;
  context2.Ovr = ovr;
  ApplyThreadChannel(context2);

  va_list args;
  va_start(args, format);

  DoLog(context2, format, args);

  va_end(args);
}

void Logger::Log(const Context& context, const char* format, ...)
{
  Context& context2 = *(Context*)&context;
  ApplyThreadChannel(context2);

  va_list args;
  va_start(args, format);

  DoLog(context2, format, args);

  va_end(args);
}

void Logger::DoLog(Context& context, const char* format, va_list args)
{
  DoAutodelete(false);

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
            context.Ovr.Add.DisableLink = true;
            chError->Display(context, buffer);
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
  Guard guard(DataLock);

  if (Default)
    callback("", Default);

  for (const auto& [name, channel] : Channels)
    callback(name, channel);
}
