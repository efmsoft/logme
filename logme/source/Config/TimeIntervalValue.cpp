#include <Logme/Utils.h>

#include <cstdlib>
#include <limits>

#include "Helper.h"

using namespace Logme;

Suffix::Suffix(const char* name, uint64_t multiplier, const char* formatName)
  : Name(name)
  , Multiplier(multiplier)
  , FormatName(formatName)
{
}

bool ParseSmartValue(
  const std::string& input
  , uint64_t& value
  , const Suffix* suffixes
  , size_t n
)
{
  std::string v = TrimSpaces(input);
  if (v.empty())
    return false;

  ToLowerAsciiInplace(v);

  uint64_t mul = 1;
  for (size_t i = 0; i < n; ++i)
  {
    const auto& s = suffixes[i];

    if (v.ends_with(s.Name) == false)
      continue;

    mul = s.Multiplier;
    v = TrimSpaces(v.substr(0, v.rfind(s.Name)));
    break;
  }

  if (v.empty())
    return false;

  char* e = nullptr;
  uint64_t val = std::strtoull(v.c_str(), &e, 10);
  if (*e)
    return false;

  if (mul != 0 && val > std::numeric_limits<uint64_t>::max() / mul)
    return false;

  value = val * mul;
  return true;
}

uint64_t SmartValue(
  const std::string& input
  , uint64_t def
  , const Suffix* suffixes
  , size_t n
)
{
  uint64_t value = 0;
  if (ParseSmartValue(input, value, suffixes, n) == false)
    return def;

  return value;
}

std::string FormatSmartValue(
  uint64_t value
  , const Suffix* suffixes
  , size_t n
)
{
  for (size_t i = 0; i < n; ++i)
  {
    const auto& s = suffixes[i];
    if (s.FormatName == nullptr)
      continue;

    if (s.Multiplier == 0)
      continue;

    if (value == 0 && s.Multiplier != 1)
      continue;

    if (value % s.Multiplier != 0)
      continue;

    return std::to_string(value / s.Multiplier) + s.FormatName;
  }

  return std::to_string(value);
}

static Suffix IntervalSuffixes[] =
{
  {"weeks", 7ULL * 24ULL * 60ULL * 60ULL * 1000ULL, "w"},
  {"week", 7ULL * 24ULL * 60ULL * 60ULL * 1000ULL},
  {"w", 7ULL * 24ULL * 60ULL * 60ULL * 1000ULL},

  {"days", 24ULL * 60ULL * 60ULL * 1000ULL, "d"},
  {"day", 24ULL * 60ULL * 60ULL * 1000ULL},
  {"d", 24ULL * 60ULL * 60ULL * 1000ULL},

  {"hours", 60ULL * 60ULL * 1000ULL, "h"},
  {"hour", 60ULL * 60ULL * 1000ULL},
  {"h", 60ULL * 60ULL * 1000ULL},

  {"minutes", 60ULL * 1000ULL, "min"},
  {"minute", 60ULL * 1000ULL},
  {"min", 60ULL * 1000ULL},
  {"m", 60ULL * 1000ULL},

  {"seconds", 1000ULL, "s"},
  {"second", 1000ULL},
  {"sec", 1000ULL},
  {"s", 1000ULL},

  {"milliseconds", 1ULL},
  {"millisecond", 1ULL},
  {"ms", 1ULL, "ms"},
};

bool Logme::ParseInterval(const std::string& input, uint64_t& value)
{
  return ParseSmartValue(
    input
    , value
    , IntervalSuffixes
    , sizeof(IntervalSuffixes) / sizeof(IntervalSuffixes[0])
  );
}

uint64_t Logme::GetInterval(const std::string& input, uint64_t def)
{
  return SmartValue(
    input
    , def
    , IntervalSuffixes
    , sizeof(IntervalSuffixes) / sizeof(IntervalSuffixes[0])
  );
}

std::string Logme::FormatInterval(uint64_t value)
{
  return FormatSmartValue(
    value
    , IntervalSuffixes
    , sizeof(IntervalSuffixes) / sizeof(IntervalSuffixes[0])
  );
}

#ifdef USE_JSONCPP

uint64_t SmartValue(
  const Json::Value& root
  , const char* option
  , uint64_t def
  , const Suffix* suffixes
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

  return SmartValue(value.asString(), def, suffixes, n);
}

uint64_t Logme::GetInterval(const Json::Value& root, const char* option, uint64_t def)
{
  return SmartValue(
    root
    , option
    , def
    , IntervalSuffixes
    , sizeof(IntervalSuffixes) / sizeof(IntervalSuffixes[0])
  );
}

#endif
