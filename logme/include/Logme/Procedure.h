#pragma once

#include <chrono>
#include <stdarg.h>
#include <string>

#include <Logme/ID.h>
#include <Logme/Printer.h>
#include <Logme/Types.h>

namespace Logme
{
  class Procedure
  {
  protected:
    const Context& BeginContext;
    const ID& Channel;
    Printer* RValPrinter;
    std::string Duration;
    
    std::chrono::time_point<std::chrono::system_clock> Begin;

  public:
    LOGMELNK Procedure(
      const Context& context
      , Printer* printer
      , const ID& ch
      , const char* format = nullptr
      , ...
    );

    LOGMELNK Procedure(
      const Context& context
      , Printer* printer
      , ChannelPtr pch
      , const char* format = nullptr
      , ...
    );

    LOGMELNK Procedure(
      const Context& context
      , Printer* printer
      , const char* format = nullptr
      , ...
    );

    LOGMELNK virtual ~Procedure();

    LOGMELNK void Print(bool begin, const char* text = nullptr);
    LOGMELNK static std::string Format(const char* format, va_list args);
    LOGMELNK static const char* AppendDuration(struct Context& context);
  };
}

#define NoRetval  Logme::None

#if _LOGME_ACTIVE

#define _LogmeP(level, retval, ...) \
  unsigned char _procStorage[sizeof(Logme::PrinterT<int>)]; \
  const Logme::Context& _procContext = LOGME_CONTEXT(level, &CH, &SUBSID, ## __VA_ARGS__); \
  Logme::Procedure logme_proc(_procContext, Logme::CreatePrinter(retval, _procStorage), ## __VA_ARGS__)

#define _LogmePV(level, ...) \
  const Logme::Context& _procContext = LOGME_CONTEXT(level, &CH, &SUBSID, ## __VA_ARGS__); \
  Logme::Procedure logme_proc(_procContext, nullptr, __VA_ARGS__)
 
#else
#define _LogmeP(level, retval, ...)
#define _LogmePV(level)
#endif

#define LogmeP(retval, ...) \
  _LogmeP(Logme::Level::LEVEL_INFO, retval, ## __VA_ARGS__)

#define LogmePV(...) \
  _LogmePV(Logme::Level::LEVEL_INFO, ## __VA_ARGS__)

#define LogmePI(retval, ...) \
  _LogmeP(Logme::Level::LEVEL_INFO, retval, ## __VA_ARGS__)

#define LogmePVI(...) \
  _LogmePV(Logme::Level::LEVEL_INFO, ## __VA_ARGS__)

#define LogmePD(retval, ...) \
  _LogmeP(Logme::Level::LEVEL_DEBUG, retval, ## __VA_ARGS__)

#define LogmePVD(...) \
  _LogmePV(Logme::Level::LEVEL_DEBUG, ## __VA_ARGS__)

#define LogmePW(retval, ...) \
  _LogmeP(Logme::Level::LEVEL_WARN, retval, ## __VA_ARGS__)

#define LogmePVW(...) \
  _LogmePV(Logme::Level::LEVEL_WARN, ## __VA_ARGS__)

#define LogmePE(retval, ...) \
  _LogmeP(Logme::Level::LEVEL_ERROR, retval, ## __VA_ARGS__)

#define LogmePVE(...) \
  _LogmePV(Logme::Level::LEVEL_ERROR, ## __VA_ARGS__)

#define LogmePC(retval, ...) \
  _LogmeP(Logme::Level::LEVEL_CRITICAL, retval, ## __VA_ARGS__)

#define LogmePVC(...) \
  _LogmePV(Logme::Level::LEVEL_CRITICAL, ## __VA_ARGS__)
