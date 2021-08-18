#include <Logme/Logger.h>
#include <Logme/Backend/ConsoleBackend.h>
#include <Logme/Backend/DebugBackend.h>
#include <Logme/File/exe_path.h>

#include "StringHelpers.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <stdarg.h>

#pragma warning(disable : 6255)

using namespace Logme;

LoggerPtr Logme::Instance = std::make_shared<Logger>();

Logger::Logger()
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
}

const std::string& Logger::GetHomeDirectory() const
{
  return HomeDirectory;
}

ChannelPtr Logger::GetChannel(const ID& id)
{
  Guard guard(DataLock);

  if (*id.Name == '\0')
    return Default;

  for (auto it = Channels.begin(); it != Channels.end(); ++it)
  {
    auto& v = *it;
    if (*v == id.Name)
      return v;
  }
  return CreateChannelInternal(id, OutputFlags());
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
  return Stream(shared_from_this(), context);
}

Stream Logger::Log(const Context& context, const ID& id)
{
  Context& context2 = *(Context *)&context;
  context2.Channel = &id;

  return Stream(shared_from_this(), context);
}

Stream Logger::Log(const Context& context, const ID& id, const char* format, ...)
{
  va_list args;
  va_start(args, format);

  Context& context2 = *(Context *)&context;
  context2.Channel = &id;

  DoLog(context2, format, args);

  va_end(args);

  return Stream(shared_from_this(), context);
}

Stream Logger::Log(const Context& context, const char* format, ...)
{
  va_list args;
  va_start(args, format);

  DoLog(*(Context *)&context, format, args);

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
    size_t size = std::max(4096U, (unsigned int)(strlen(format) + 512));

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
