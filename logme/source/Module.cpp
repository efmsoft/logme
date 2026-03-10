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
    const char* p0 = strrchr(file, '/');
    const char* p1 = strrchr(file, '\\');
    const char* p = p0;

    if (p == nullptr || (p1 && p1 > p))
      p = p1;

    if (p)
      return p + 1;

    return file;
  }
}

Module::Module(const char* file)
  : FullName(file)
  , ShortName(file)
{
  if (file == nullptr)
  {
    FullName = "";
    ShortName = "";
    return;
  }

  thread_local ModuleCacheEntry Cache[4] = {};
  thread_local size_t CacheIndex = 0;

  for (size_t i = 0; i < 4; ++i)
  {
    if (Cache[i].FullName == file)
    {
      ShortName = Cache[i].ShortName;
      return;
    }
  }

  ShortName = GetShortModuleName(file);

  Cache[CacheIndex].FullName = file;
  Cache[CacheIndex].ShortName = ShortName;

  ++CacheIndex;

  if (CacheIndex >= 4)
    CacheIndex = 0;
}