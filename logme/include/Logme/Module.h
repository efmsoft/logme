#pragma once

#include <Logme/Types.h>

namespace Logme
{
  struct Module
  {
    const char* FullName;
    const char* ShortName;

  public:
    LOGMELNK Module(const char* file);
  };
}
