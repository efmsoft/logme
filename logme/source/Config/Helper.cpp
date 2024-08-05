#include <algorithm>

#include "Helper.h"

#ifdef USE_JSONCPP
#include <json/json.h>
#endif

using namespace Logme;

#ifdef _WIN32
  #define IS_WINDOWS true
  #define IS_MACOS false
  #define IS_LINUX false
#elif defined(__APPLE__)
  #define IS_MACOS true
  #define IS_WINDOWS false
  #define IS_LINUX false
#elif defined(__linux__)
  #define IS_LINUX true
  #define IS_MACOS false
  #define IS_WINDOWS false
#endif

#ifdef _DEBUG
  #define IS_DEBUG true
  #define IS_RELEASE false
#else
  #define IS_DEBUG false
  #define IS_RELEASE true
#endif

static NAMED_VALUE PlatformValues[] =
{
  {"", true},
  {"any", true},
  {"none", false},

  {"windows", IS_WINDOWS},
  {"win", IS_WINDOWS},
  {"win32", IS_WINDOWS},

  {"mac", IS_MACOS},
  {"macos", IS_MACOS},
  {"apple", IS_MACOS},

  {"linux", IS_LINUX},

  {nullptr, 0}
};

static NAMED_VALUE BuildValues[] =
{
  {"", true},
  {"any", true},
  {"none", false},

  {"debug", IS_DEBUG},
  {"checked", IS_DEBUG},

  {"release", IS_RELEASE},

  {nullptr, 0}
};

std::string ReadFile(const fs::path& path)
{
  std::ifstream f(path, std::ios::in | std::ios::binary);

  const auto sz = fs::file_size(path);

  std::string result((unsigned int)sz, '\0');
  f.read(result.data(), sz);

  return result;
}

bool Name2Value(
  const std::string& name
  , bool caseSensitive
  , const NAMED_VALUE* pv
  , int& v
)
{
  std::string s(name);

  if (caseSensitive == false)
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);

  for (; pv->Name; pv++)
  {
    if (s != pv->Name)
      continue;

    v = pv->Value;
    return true;
  }

  return false;
}

bool CheckBuild(const std::string& build, bool& ok)
{
  int v = false;
  if (!Name2Value(build, false, BuildValues, v))
  {
    LogmeE("unsupported type of \"build\": %s", build.c_str());
    return false;
  }

  ok = v != false;
  return true;
}

bool CheckPlatform(const std::string& build, bool& ok)
{
  int v = false;
  if (!Name2Value(build, false, PlatformValues, v))
  {
    LogmeE("unsupported \"platform\": %s", build.c_str());
    return false;
  }

  ok = v != false;
  return true;
}

#ifdef USE_JSONCPP

bool GetConfigSection(
  const Json::Value& root
  , const std::string& section
  , Json::Value& config
)
{
  config = root;
  std::istringstream stream(section);

  std::string member;
  while (std::getline(stream, member, '|')) 
  {
    if (member.empty())
      continue;

    if (!config.isMember(member))
    {
      LogmeE(CHINT, "No such member: \"%s\"", member.c_str());
      return false;
    }

    config = config[member];
  }

  return true;
}

#endif
