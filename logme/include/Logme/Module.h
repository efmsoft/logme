#pragma once

namespace Logme
{
  struct Module
  {
    const char* FullName;
    const char* ShortName;

  public:
    Module(const char* file);
  };
}
