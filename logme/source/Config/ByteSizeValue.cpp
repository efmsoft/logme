#include <Logme/Utils.h>

#include "Helper.h"

#ifdef USE_JSONCPP

using namespace Logme;

uint64_t GetByteSize(const Json::Value& root, const char* option, uint64_t def)
{
  static Suffix suffixes[] =
  {
    {"gb", 1024 * 1024 * 1024},
    {"gib", 1024 * 1024 * 1024},
    {"g", 1024 * 1024 * 1024},

    {"mb", 1024 * 1024},
    {"mib", 1024 * 1024},
    {"m", 1024 * 1024},

    {"kb", 1024},
    {"kib", 1024},
    {"k", 1024},

    {"b", 1},
  };

  return SmartValue(
    root
    , option
    , def
    , suffixes
    , sizeof(suffixes) / sizeof(suffixes[0])
  );
}

#endif
