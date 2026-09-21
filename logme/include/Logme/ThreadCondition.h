#pragma once

#include <memory>

namespace Logme
{
  class Logger;
  typedef std::shared_ptr<Logger> LoggerPtr;

  // RAII guard mirroring ThreadChannel/ThreadSubsystem: while alive, adds
  // `condition` to what every LogmeD/LogmeI/LogmeW/LogmeE call on this
  // thread is gated by (see LoggerCondition() in Logger.h), restoring
  // whatever this thread had set (or had unset) before construction once
  // destroyed. A class whose own LoggerCondition() member override already
  // shadows the global default is unaffected either way.
  class ThreadCondition
  {
    LoggerPtr Logger;
    bool HadPrev;
    bool PrevCondition;

  public:
    LOGMELNK ThreadCondition(LoggerPtr logger, bool condition);
    LOGMELNK ~ThreadCondition();
  };
}
