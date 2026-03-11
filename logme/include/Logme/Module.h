#pragma once

#include <Logme/Types.h>

namespace Logme
{
  struct Module
  {
    const char* FullName;
  
  private:
    const char* ShortName;

  public:
    LOGMELNK Module(const char* file);
    LOGMELNK const char* GetShortName();
  };
}
