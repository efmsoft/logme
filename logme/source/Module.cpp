#include <Logme/Module.h>

#include <string.h>

using namespace Logme;

Module::Module(const char* file)
  : FullName(file)
  , ShortName(file)
{
  if (file == nullptr)
  {
    FullName = "";
    ShortName = "";
  }
  else
  {
    const char* p0 = strrchr(file, '/');
    const char* p1 = strrchr(file, '\\');

    if (p0 == nullptr)
      p0 = p1;
    else if (p1 && p1 < p0)
      p0 = p1;

    if (p0)
      ShortName = ++p0;
  }
}
