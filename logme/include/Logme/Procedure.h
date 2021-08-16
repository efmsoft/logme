#pragma once

#include <chrono>
#include <stdarg.h>
#include <string>

#include "ID.h"
#include "Printer.h"
#include "Types.h"

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
#define _LogmeProc(level, retval, ...) \
  unsigned char _procStorage[sizeof(Logme::PrinterT<int>)]; \
  const Logme::Context& _procContext = LOGME_CONTEXT(level, &CH); \
  Logme::Procedure logme_proc(_procContext, Logme::CreatePrinter(retval, _procStorage), ## __VA_ARGS__)
#else
#define _LogmeProc(level, retval, ...)
#endif

#define LogmeProcD(retval, ...) \
  _LogmeProc(Logme::Level::LEVEL_DEBUG, retval, ## __VA_ARGS__)

#define LogmeProc(retval, ...) \
  _LogmeProc(Logme::Level::LEVEL_INFO, retval, ## __VA_ARGS__)

#define LogmeProcI(retval, ...) \
  _LogmeProc(Logme::Level::LEVEL_INFO, retval, ## __VA_ARGS__)

#define LogmeProcW(retval, ...) \
  _LogmeProc(Logme::Level::LEVEL_WARN, retval, ## __VA_ARGS__)
