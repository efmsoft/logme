#pragma once

#include <memory>
#include <mutex>
#include <stdint.h>

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

  enum ConsoleStream
  {
    STREAM_ALL2COUT,
    STREAM_WARNCERR,
    STREAM_ERRCERR,
    STREAM_CERRCERR,
    STREAM_ALL2CERR,
  };

  typedef std::lock_guard<std::mutex> Guard;
  typedef void* HANDLE;

  class Logger;
  typedef std::shared_ptr<Logger> LoggerPtr;

  template<typename T> struct HexType
  {
    T Value;

    HexType(T v) : Value(v)
    {
    } 

    operator T () const
    {
      return Value;
    }

    operator T& ()
    {
      return Value;
    }
  };

  #define xint8_t HexType<int8_t>
  #define xuint8_t HexType<uint8_t>
  #define xint16_t HexType<int16_t>
  #define xuint16_t HexType<uint16_t>
  #define xint32_t HexType<int32_t>
  #define xuint32_t HexType<uint32_t>
  #define xint64_t HexType<int64_t>
  #define xuint64_t HexType<uint64_t>
}

#define _LOGME_DROP_COMMA(...) , ##__VA_ARGS__

#if !defined(_DEBUG) && !defined(LOGME_INRELEASE)
#define _LOGME_ACTIVE 0
#else
#define _LOGME_ACTIVE 1
#endif
