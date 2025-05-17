#include <Logme/Utils.h>

#include "Helper.h"

#ifdef USE_JSONCPP

using namespace Logme;

uint64_t SmartValue(
  const Json::Value& root
  , const char* option
  , uint64_t def
  , Suffix* suffixes
  , size_t n
)
{
  if (root.isMember(option) == false)
    return def;

  auto& value = root[option];

  if (value.isInt())
    return value.asInt();

  if (value.isString() == false)
    return def;

  std::string v = TrimSpaces(value.asString());
  ToLowerAsciiInplace(v);

  int mul = 1;
  for (size_t i = 0; i < n; ++i)
  {
    auto& s = suffixes[i];

    if (v.ends_with(s.Name) == false)
      continue;

    mul = s.Multiplier;
    v = TrimSpaces(v.substr(0, v.rfind(s.Name)));
    break;
  }

  char* e = nullptr;
  uint64_t val = std::strtoull (v.c_str(), &e, 10);
  if (*e)
    return def;

  return val * mul;
}

uint64_t Logme::GetInterval(const Json::Value& root, const char* option, uint64_t def)
{
  static Suffix suffixes[] =
  {
    {"weeks", 7 * 24 * 60 * 60 * 1000},
    {"week", 7 * 24 * 60 * 60 * 1000},
    {"w", 7 * 24 * 60 * 60 * 1000},

    {"days", 24 * 60 * 60 * 1000},
    {"day", 24 * 60 * 60 * 1000},
    {"d", 24 * 60 * 60 * 1000},

    {"hours", 60 * 60 * 1000},
    {"hour", 60 * 60 * 1000},
    {"h", 60 * 60 * 1000},

    {"minutes", 60 * 1000},
    {"minute", 60 * 1000},
    {"min", 60 * 1000},
    {"m", 60 * 1000},

    {"seconds", 1000},
    {"second", 1000},
    {"sec", 1000},
    {"s", 1000},

    {"milliseconds", 1},
    {"millisecond", 1},
    {"ms", 1},
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
