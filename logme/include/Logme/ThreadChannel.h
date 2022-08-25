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
    ThreadChannel(LoggerPtr logger, const ID& ch);
    ~ThreadChannel();
  };
}