#include <Logme/Module.h>

#include <string.h>

using namespace Logme;

namespace
{
  struct ModuleCacheEntry
  {
    const char* FullName;
    const char* ShortName;
  };

  static const char* GetShortModuleName(const char* file)
  {
    const char* last = file;
    for (const char* p = file; *p; ++p)
    {
      if (*p == '/' || *p == '\\')
        last = p + 1;
    }

    return last;
  }
}

Module::Module(const char* file)
  : FullName(file)
  , ShortName(nullptr)
{
  if (file == nullptr)
  {
    FullName = "";
    ShortName = "";
  }
}

const char* Module::GetShortName()
{
  if (ShortName)
    return ShortName;

  thread_local ModuleCacheEntry Cache[4] = {};
  thread_local size_t CacheIndex = 0;

  for (size_t i = 0; i < 4; ++i)
  {
    if (Cache[i].FullName == FullName)
    {
      ShortName = Cache[i].ShortName;
      return ShortName;
    }
  }

  ShortName = GetShortModuleName(FullName);

  Cache[CacheIndex].FullName = FullName;
  Cache[CacheIndex].ShortName = ShortName;

  ++CacheIndex;

  if (CacheIndex >= 4)
    CacheIndex = 0;

  return ShortName;
}