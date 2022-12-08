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

  #pragma warning(push)
  #pragma warning(disable : 26812)
  static constexpr const Level DEFAULT_LEVEL = Level::LEVEL_INFO;
  #pragma warning(pop)

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

#ifdef _MSC_VER
#define _LOGME_NONEMPTY(...) ,
#else
#define _LOGME_EMPTYFIRST(x,...) _LOGME_A x (_LOGME_B)
#define _LOGME_A(x) x()
#define _LOGME_B() ,

#define _LOGME_EMPTY(...) _LOGME_C(_LOGME_EMPTYFIRST(__VA_ARGS__) _LOGME_SINGLE(__VA_ARGS__))
#define _LOGME_C(...) _LOGME_D(__VA_ARGS__)
#define _LOGME_D(x,...) __VA_ARGS__

#define _LOGME_SINGLE(...) _LOGME_E(__VA_ARGS__, _LOGME_B)
#define _LOGME_E(x,y,...) _LOGME_C(y(),)

#define _LOGME_NONEMPTY(...) _LOGME_F(_LOGME_EMPTY(__VA_ARGS__) _LOGME_D, _LOGME_B)
#define _LOGME_F(...) _LOGME_G(__VA_ARGS__)
#define _LOGME_G(x,y,...) y()
#endif

#ifndef _LOGME_ACTIVE
  #if !defined(_DEBUG) && !defined(LOGME_INRELEASE)
    #define _LOGME_ACTIVE 0
  #else
    #define _LOGME_ACTIVE 1
  #endif
#endif
