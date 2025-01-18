#pragma once

#include <memory>
#include <Logme/Override.h>

namespace Logme
{
  class Logger;
  typedef std::shared_ptr<Logger> LoggerPtr;

  class ThreadOverride
  {
    LoggerPtr Logger;
    Override PrevOverride;

  public:
    LOGMELNK ThreadOverride(LoggerPtr logger, const Override& ovr);
    LOGMELNK ~ThreadOverride();
  };
}