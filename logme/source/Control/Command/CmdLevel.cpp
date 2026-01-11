#include <algorithm>
#include <cctype>
#include <string>

#include <Logme/Channel.h>
#include <Logme/Logger.h>

#include "../CommandRegistrar.h"

using namespace Logme;

COMMAND_DESCRIPTOR2("level", Logger::CommandLevel);

struct NamedValue
{
  const char* Name;
  int Value;
};

static NamedValue LevelValues[] =
{
  {"", DEFAULT_LEVEL},
  {"info", LEVEL_INFO},
  {"information", LEVEL_INFO},
  {"debug", LEVEL_DEBUG},
  {"warn", LEVEL_WARN},
  {"warning", LEVEL_WARN},
  {"err", LEVEL_ERROR},
  {"error", LEVEL_ERROR},
  {"crit", LEVEL_CRITICAL},
  {"ctitical", LEVEL_CRITICAL},
  {nullptr, 0}
};

static std::string ToLower(std::string s)
{
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
  return s;
}

static bool FindNamed(const NamedValue* values, const std::string& s, int& out)
{
  std::string v = ToLower(s);
  for (size_t i = 0; values[i].Name != nullptr; ++i)
  {
    if (v == values[i].Name)
    {
      out = values[i].Value;
      return true;
    }
  }
  return false;
}

static bool GetChannelFromArgs(
  Logme::StringArray& arr
  , size_t& index
  , ChannelPtr& ch
  , std::string& error
)
{
  if (index + 1 < arr.size() && arr[index] == "--channel")
  {
    const std::string& name = arr[index + 1];
    if (name.empty())
    {
      error = "invalid channel name";
      return false;
    }

    ch = Logme::Instance->GetExistingChannel(ID{name.c_str()});
    if (ch == nullptr)
    {
      error = "no such channel: " + name;
      return false;
    }

    index += 2;
    return true;
  }

  ch = Logme::Instance->GetExistingChannel(::CH);
  if (ch == nullptr)
  {
    error = "no default channel";
    return false;
  }

  return true;
}

bool Logger::CommandLevel(Logme::StringArray& arr, std::string& response)
{
  size_t index = 1;
  ChannelPtr ch;
  std::string error;

  std::lock_guard guard(Instance->DataLock);

  if (!GetChannelFromArgs(arr, index, ch, error))
  {
    response = "error: " + error;
    return true;
  }

  if (index >= arr.size())
  {
    response = std::string("Level: ") + GetLevelName(ch->GetFilterLevel());
    return true;
  }

  int level = 0;
  if (!FindNamed(LevelValues, arr[index], level))
  {
    response = "error: invalid level: " + arr[index];
    return true;
  }

  ch->SetFilterLevel((Level)level);
  response = std::string("Level: ") + GetLevelName(ch->GetFilterLevel()) + "\nok";
  return true;
}
