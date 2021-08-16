#pragma once

#include <memory>
#include <mutex>

namespace Logme
{
  enum Result
  {
    RC_NOERROR,
    RC_NO_ACCESS,
  };

  enum Level
  {
    LEVEL_DEBUG,
    LEVEL_INFO,
    LEVEL_WARN,
    LEVEL_ERROR,
    LEVEL_CRITICAL,
  };

  static constexpr const Level DEFAULT_LEVEL = Level::LEVEL_INFO;

  enum Detality
  {
    DETALITY_NONE,
    DETALITY_SHORT,
    DETALITY_FULL
  };

  enum TimeFormat
  {
    TIME_FORMAT_NONE,
    TIME_FORMAT_LOCAL,
    TIME_FORMAT_TZ,
    TIME_FORMAT_UTC,
  };

  typedef std::lock_guard<std::mutex> Guard;
  typedef void* HANDLE;

  class Logger;
  typedef std::shared_ptr<Logger> LoggerPtr;
}

#define _LOGME_DROP_COMMA(a, ...) a , ## __VA_ARGS__

#if !defined(_DEBUG) && !defined(LOGME_INRELEASE)
#define _LOGME_ACTIVE 0
#else
#define _LOGME_ACTIVE 1
#endif
