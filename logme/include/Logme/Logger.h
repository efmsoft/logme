#pragma once

#include <Logme/Channel.h>
#include <Logme/Stream.h>

#include <memory>
#include <mutex>
#include <string>

namespace Logme
{
  class Logger : public std::enable_shared_from_this<Logger>
  {
    std::mutex DataLock;

    ChannelArray Channels;
    ChannelPtr Default;
    std::string HomeDirectory;

    int IDGenerator;

  public:
    Logger();
    virtual ~Logger();

    virtual Stream DoLog(Context& context, const char* format, va_list args);

    Stream Log(const Context& context);

    Stream Log(const Context& context, const Override& ovr);
    Stream Log(const Context& context, const ID& id);
    Stream Log(const Context& context, const ID& id, const Override& ovr);

    Stream Log(const Context& context, const ID& id, const char* format, ...);
    Stream Log(const Context& context, const Override& ovr, const char* format, ...);
    Stream Log(const Context& context, const ID& id, const Override& ovr, const char* format, ...);

    Stream Log(const Context& context, const char* format, ...);

    ChannelPtr GetChannel(const ID& id);

    ChannelPtr CreateChannel(
      const ID& id
      , const OutputFlags& flags = OutputFlags()
      , Level level = DEFAULT_LEVEL
    );
    ChannelPtr CreateChannel(
      const OutputFlags& flags = OutputFlags()
      , Level level = DEFAULT_LEVEL
    );
    void DeleteChannel(const ID& id);

    const std::string& GetHomeDirectory() const;

  protected:
    ChannelPtr CreateChannelInternal(
      const ID& id
      , const OutputFlags& flags
      , Level level = DEFAULT_LEVEL
    );
  };

  typedef std::shared_ptr<Logger> LoggerPtr;
  extern LoggerPtr Instance;
}

