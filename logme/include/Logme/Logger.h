#pragma once

#include <Logme/Channel.h>

#include <mutex>
#include <string>

namespace Logme
{
  class Logger
  {
    std::mutex DataLock;

    ChannelArray Channels;
    ChannelPtr Default;
    std::string HomeDirectory;

  public:
    Logger();
    virtual ~Logger();

    virtual void Log(Context& context, const char* format, va_list args);

    void Log(const Context& context, const ID& id, const char* format, ...);
    void Log(const Context& context, const char* format, ...);

    ChannelPtr GetChannel(const ID& id);

    ChannelPtr CreateChannel(
      const ID& id
      , const OutputFlags& flags = OutputFlags()
      , Level level = DEFAULT_LEVEL
    );

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

