#pragma once

#include <memory>

#include <Logme/SafeID.h>

namespace Logme
{
  class Logger;
  typedef std::shared_ptr<Logger> LoggerPtr;

  class ThreadChannel
  {
    LoggerPtr Logger;
    SafeID PrevChannel;

  public:
    LOGMELNK ThreadChannel(LoggerPtr logger, const ID& ch);
    LOGMELNK ~ThreadChannel();
  };
}