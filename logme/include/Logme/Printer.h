#pragma once

#include <string>

#include <Logme/Types.h>

namespace Logme
{
  struct Printer
  {
    virtual std::string Format()
    {
      return std::string();
    }
  };

  template<typename T> struct PrinterT : public Printer
  {
    const T& RetVal;

    PrinterT(const T& retVal)
      : RetVal(retVal)
    {
    }

    std::string Format() override
    {
      LOGMELNK std::string FormatValue(const T&);
      return FormatValue(RetVal);
    }
  };

  template<typename T> Printer* CreatePrinter(const T& retVal, void* storage)
  {
    return new (storage) PrinterT<T>(retVal);
  }

  LOGMELNK extern Printer None;
}
