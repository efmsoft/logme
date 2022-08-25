#include <Logme/Logger.h>
#include <Logme/Backend/ConsoleBackend.h>
#include <Logme/Backend/DebugBackend.h>
#include <Logme/File/exe_path.h>
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
{
  Default = std::make_shared<Channel>(this, nullptr, OutputFlags(), DEFAULT_LEVEL);

  Default->AddBackend(std::make_shared<ConsoleBackend>(Default));
#ifdef _DEBUG
  Default->AddBackend(std::make_shared<DebugBackend>(Default));
#endif

  HomeDirectory = GetExecutablePath();
}

Logger::~Logger()
{
  // Backends keep shared_ptr to owner. We have to clear it to delete channel!
  Default->RemoveBackends();
  for (auto c : Channels)
  {
    c->RemoveLink();
    c->RemoveBackends();
  }
  
  Channels.clear();
  Default.reset();
}

const std::string& Logger::GetHomeDirectory() const
{
  return HomeDirectory;
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

ChannelPtr Logger::GetChannel(const ID& id)
{
  if (id.Name == nullptr || *id.Name == '\0')
    return Default;

  Guard guard(DataLock);
  for (auto it = Channels.begin(); it != Channels.end(); ++it)
  {
    auto& v = *it;
    if (*v == id.Name)
      return v;
  }
  return CreateChannelInternal(id, OutputFlags());
}

void Logger::DeleteChannel(const ID& id)
{
  ChannelPtr ch;

  do
  {
    Guard guard(DataLock);
    for (auto it = Channels.begin(); it != Channels.end(); ++it)
    {
      auto& v = *it;
      if (*v == id.Name)
      {
        ch = v;
        Channels.erase(it);

        break;
      }
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

    bool found = false;
    for (auto it = Channels.begin(); it != Channels.end(); ++it)
    {
      auto& v = *it;
      if (*v == id.Name)
      {
        found = true;
        break;
      }
    }

    if (found == false)
      return CreateChannelInternal(id, flags, level);
  }
  return ChannelPtr();
}

ChannelPtr Logger::CreateChannel(
  const ID& id
  , const OutputFlags& flags
  , Level level
)
{
  Guard guard(DataLock);
  for (auto it = Channels.begin(); it != Channels.end(); ++it)
  {
    auto& v = *it;
    if (*v == id.Name)
      return v;
  }
  return CreateChannelInternal(id, flags, level);
}

ChannelPtr Logger::CreateChannelInternal(
  const ID& id
  , const OutputFlags& flags
  , Level level
)
{
  ChannelPtr channel = std::make_shared<Channel>(this, id.Name, flags, level);
  Channels.push_back(channel);

  return channel;
}

Stream Logger::Log(const Context& context)
{
  Context& context2 = *(Context*)&context;
  ApplyThreadChannel(context2);

  return Stream(shared_from_this(), context);
}

Stream Logger::Log(const Context& context, const ID& id)
{
  Context& context2 = *(Context *)&context;
  context2.Channel = &id;

  return Stream(shared_from_this(), context);
}

Stream Logger::Log(const Context& context, const ID& id, const Override& ovr)
{
  Context& context2 = *(Context*)&context;
  context2.Channel = &id;
  context2.Ovr = ovr;

  return Stream(shared_from_this(), context);
}

Stream Logger::Log(const Context& context, const Override& ovr)
{
  Context& context2 = *(Context*)&context;
  context2.Ovr = ovr;

  ApplyThreadChannel(context2);

  return Stream(shared_from_this(), context);
}

Stream Logger::Log(const Context& context, const ID& id, const char* format, ...)
{
  Context& context2 = *(Context*)&context;
  context2.Channel = &id;

  va_list args;
  va_start(args, format);

  DoLog(context2, format, args);

  va_end(args);

  return Stream(shared_from_this(), context);
}

Stream Logger::Log(
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

  return Stream(shared_from_this(), context);
}

Stream Logger::Log(
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

  return Stream(shared_from_this(), context);
}

Stream Logger::Log(const Context& context, const char* format, ...)
{
  Context& context2 = *(Context*)&context;
  ApplyThreadChannel(context2);

  va_list args;
  va_start(args, format);

  DoLog(context2, format, args);

  va_end(args);

  return Stream(shared_from_this(), context);
}

Stream Logger::DoLog(Context& context, const char* format, va_list args)
{
  ChannelPtr ch = GetChannel(*context.Channel);
  
  if (ch == nullptr)
    return Stream(shared_from_this(), context);

  if (format)
  {
    size_t size = std::max(16384U, (unsigned int)(strlen(format) + 512));

    char* buffer = (char*)alloca(size);

    buffer[0] = '\0';
    buffer[size - 1] = '\0';

    int rc = vsnprintf(buffer + strlen(buffer), size - 1, format, args);
    if (rc == -1)
      strcpy_s(buffer, size - 1, "[format error]");

    ch->Display(context, buffer);

    if (context.Output)
      *context.Output = buffer;
  }
  return Stream(shared_from_this(), context);
}
