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
  Logme::Procedure logme_proc(LOGME_CONTEXT(level, &CH, &SUBSID, ## __VA_ARGS__), Logme::CreatePrinter(retval, _procStorage), ## __VA_ARGS__)

#define _LogmePV(level, ...) \
  Logme::Procedure logme_proc(LOGME_CONTEXT(level, &CH, &SUBSID, ## __VA_ARGS__), nullptr)
#else
#define _LogmeP(level, retval, ...)
#define _LogmePV(level)
#endif

#define LogmeP(retval, ...) \
  _LogmeP(Logme::Level::LEVEL_INFO, retval, ## __VA_ARGS__)

#define LogmePV(...) \
  _LogmePV(Logme::Level::LEVEL_INFO, ## __VA_ARGS__)
