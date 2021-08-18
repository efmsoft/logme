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
    Procedure(
      const Context& context
      , Printer* printer
      , const ID& ch
      , const char* format = nullptr
      , ...
    );

    Procedure(
      const Context& context
      , Printer* printer
      , const char* format = nullptr
      , ...
    );

    virtual ~Procedure();

    void Print(bool begin, const char* text = nullptr);
    static std::string Format(const char* format, va_list args);
    static const char* AppendDuration(struct Context& context);
  };
}

#define NoRetval  Logme::None

#if _LOGME_ACTIVE
#define _LogmeP(level, retval, ...) \
  unsigned char _procStorage[sizeof(Logme::PrinterT<int>)]; \
  const Logme::Context& _procContext = LOGME_CONTEXT(level, &CH); \
  Logme::Procedure logme_proc(_procContext, Logme::CreatePrinter(retval, _procStorage), ## __VA_ARGS__)
#else
#define _LogmeP(level, retval, ...)
#endif

#define LogmeP(retval, ...) \
  _LogmeP(Logme::Level::LEVEL_INFO, retval, ## __VA_ARGS__)
