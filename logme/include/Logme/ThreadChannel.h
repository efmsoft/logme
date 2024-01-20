#pragma once

#include <memory>

namespace Logme
{
  class Logger;
  typedef std::shared_ptr<Logger> LoggerPtr;

  class ThreadChannel
  {
    LoggerPtr Logger;
    ID PrevChannel;

  public:
    LOGMELNK ThreadChannel(LoggerPtr logger, const ID& ch);
    LOGMELNK ~ThreadChannel();
  };
}