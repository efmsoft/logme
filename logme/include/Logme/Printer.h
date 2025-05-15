#pragma once

#include <string>

#include <Logme/Types.h>

template<typename T> std::string FormatValue(const T&);

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
      return ::FormatValue(RetVal);
    }
  };

  template<typename T> Printer* CreatePrinter(const T& retVal, void* storage)
  {
    return new (storage) PrinterT<T>(retVal);
  }

  LOGMELNK extern Printer None;
}
