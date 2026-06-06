#include <Logme/Utils.h>

#include "Helper.h"

using namespace Logme;

static Suffix ByteSizeSuffixes[] =
{
  {"gb", 1024ULL * 1024ULL * 1024ULL, "Gb"},
  {"gib", 1024ULL * 1024ULL * 1024ULL},
  {"g", 1024ULL * 1024ULL * 1024ULL},

  {"mb", 1024ULL * 1024ULL, "Mb"},
  {"mib", 1024ULL * 1024ULL},
  {"m", 1024ULL * 1024ULL},

  {"kb", 1024ULL, "Kb"},
  {"kib", 1024ULL},
  {"k", 1024ULL},

  {"b", 1ULL, "b"},
};

bool Logme::ParseByteSize(const std::string& input, uint64_t& value)
{
  return ParseSmartValue(
    input
    , value
    , ByteSizeSuffixes
    , sizeof(ByteSizeSuffixes) / sizeof(ByteSizeSuffixes[0])
  );
}

uint64_t Logme::GetByteSize(const std::string& input, uint64_t def)
{
  return SmartValue(
    input
    , def
    , ByteSizeSuffixes
    , sizeof(ByteSizeSuffixes) / sizeof(ByteSizeSuffixes[0])
  );
}

std::string Logme::FormatByteSize(uint64_t value)
{
  return FormatSmartValue(
    value
    , ByteSizeSuffixes
    , sizeof(ByteSizeSuffixes) / sizeof(ByteSizeSuffixes[0])
  );
}

#ifdef USE_JSONCPP

uint64_t Logme::GetByteSize(const Json::Value& root, const char* option, uint64_t def)
{
  return SmartValue(
    root
    , option
    , def
    , ByteSizeSuffixes
    , sizeof(ByteSizeSuffixes) / sizeof(ByteSizeSuffixes[0])
  );
}

#endif
