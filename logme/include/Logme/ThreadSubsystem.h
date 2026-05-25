#pragma once

#include <memory>

#include <Logme/SID.h>

namespace Logme
{
  class Logger;
  typedef std::shared_ptr<Logger> LoggerPtr;

  class ThreadSubsystem
  {
    LoggerPtr Logger;
    SID PrevSubsystem;

  public:
    LOGMELNK ThreadSubsystem(LoggerPtr logger, const SID& sid);
    LOGMELNK ~ThreadSubsystem();
  };
}
