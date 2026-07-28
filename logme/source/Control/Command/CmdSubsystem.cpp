#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>

#include <Logme/Logger.h>
#include <Logme/SID.h>
#include <Logme/Utils.h>

#include "../CommandRegistrar.h"

using namespace Logme;

COMMAND_DESCRIPTOR2("subsystem", Logger::CommandSubsystem);

static std::string SidToString(uint64_t sid)
{
  char buf[9] = {0};
  std::memcpy(buf, &sid, 8);
  buf[8] = 0;

  size_t len = 0;
  while (len < 8 && buf[len] != 0)
    ++len;

  return std::string(buf, len);
}

static bool HasSubsystem(const std::vector<uint64_t>& arr, uint64_t id)
{
  auto it = std::lower_bound(arr.begin(), arr.end(), id);
  return (it != arr.end() && *it == id);
}


static bool ParseLevelName(const std::string& name, Level& level)
{
  std::string value(name);
  std::transform(
    value.begin()
    , value.end()
    , value.begin()
    , [](unsigned char c)
      {
        return static_cast<char>(std::tolower(c));
      }
  );

  if (value == "debug")
    level = LEVEL_DEBUG;
  else if (value == "info" || value == "information")
    level = LEVEL_INFO;
  else if (value == "warn" || value == "warning")
    level = LEVEL_WARN;
  else if (value == "err" || value == "error")
    level = LEVEL_ERROR;
  else if (value == "crit" || value == "critical" || value == "ctitical")
    level = LEVEL_CRITICAL;
  else
    return false;

  return true;
}

static void PrintSubsystemList(
  std::string& response
  , const char* title
  , const std::vector<uint64_t>& arr
)
{
  if (arr.empty())
  {
    response += title;
    response += ": none\n";
    return;
  }

  response += title;
  response += ":\n";
  for (auto id : arr)
    response += "  " + SidToString(id) + "\n";
}

bool Logger::CommandSubsystem(Logme::StringArray& arr, std::string& response)
{
  std::lock_guard guard(Instance->DataLock);

  auto findSubsystemLevel = [](auto& levels, uint64_t subsystem)
  {
    return std::lower_bound(
      levels.begin()
      , levels.end()
      , subsystem
      , [](const auto& item, uint64_t value)
        {
          return item.Subsystem < value;
        }
    );
  };

  auto printSubsystemLevels = [&](std::string& output)
  {
    if (Instance->SubsystemLevels.empty())
    {
      output += "Subsystem levels: none\n";
      return;
    }

    output += "Subsystem levels:\n";
    for (const auto& item : Instance->SubsystemLevels)
    {
      output += "  " + SidToString(item.Subsystem);
      output += ": ";
      output += GetLevelName(item.FilterLevel);
      output += "\n";
    }
  };

  if (arr.size() == 1)
  {
    PrintSubsystemList(response, "Blocked subsystems", Instance->BlockedSubsystems);
    PrintSubsystemList(response, "Allowed subsystems", Instance->AllowedSubsystems);
    printSubsystemLevels(response);
    return true;
  }

  if (arr.size() == 2 && arr[1] == "--clear")
  {
    Instance->BlockedSubsystems.clear();
    Instance->AllowedSubsystems.clear();
    Instance->SubsystemLevels.clear();
    Instance->PublishSubsystemLevelSnapshot();
    Instance->Subsystems.clear();
    response = "ok";
    return true;
  }

  if (arr.size() == 2 && arr[1] == "--clear-blocked")
  {
    Instance->BlockedSubsystems.clear();
    response = "ok";
    return true;
  }

  if (arr.size() == 2 && arr[1] == "--clear-allowed")
  {
    Instance->AllowedSubsystems.clear();
    response = "ok";
    return true;
  }

  if (arr.size() == 2 && arr[1] == "--clear-levels")
  {
    Instance->SubsystemLevels.clear();
    Instance->PublishSubsystemLevelSnapshot();
    response = "ok";
    return true;
  }

  if (arr.size() == 4 && arr[1] == "--set-level")
  {
    const std::string& name = arr[2];
    auto sid = SID::Build(name);
    if (sid.Name == 0)
    {
      response = "error: invalid subsystem name";
      return true;
    }

    Level parsedLevel = LEVEL_INFO;
    if (!ParseLevelName(arr[3], parsedLevel))
    {
      response = "error: invalid level: " + arr[3];
      return true;
    }

    auto it = findSubsystemLevel(Instance->SubsystemLevels, sid.Name);
    if (it != Instance->SubsystemLevels.end() && it->Subsystem == sid.Name)
      it->FilterLevel = parsedLevel;
    else
      Instance->SubsystemLevels.insert(
        it
        , Logger::SubsystemLevel{sid.Name, parsedLevel}
      );

    Instance->PublishSubsystemLevelSnapshot();
    response = "ok";
    return true;
  }

  if (arr.size() == 3 && arr[1] == "--remove-level")
  {
    const std::string& name = arr[2];
    auto sid = SID::Build(name);
    if (sid.Name == 0)
    {
      response = "error: invalid subsystem name";
      return true;
    }

    auto it = findSubsystemLevel(Instance->SubsystemLevels, sid.Name);
    if (it == Instance->SubsystemLevels.end() || it->Subsystem != sid.Name)
      response = "error: no level override for subsystem: " + name;
    else
    {
      Instance->SubsystemLevels.erase(it);
      Instance->PublishSubsystemLevelSnapshot();
      response = "ok";
    }
    return true;
  }

  if (arr.size() >= 3)
  {
    const std::string& name = arr[2];
    auto sid = SID::Build(name);
    if (sid.Name == 0)
    {
      response += "error: invalid subsystem name";
      return true;
    }

    if (arr[1] == "--block")
    {
      if (HasSubsystem(Instance->BlockedSubsystems, sid.Name))
        response = "error: already blocked: " + name;
      else
      {
        auto it = std::lower_bound(Instance->BlockedSubsystems.begin(), Instance->BlockedSubsystems.end(), sid.Name);
        Instance->BlockedSubsystems.insert(it, sid.Name);
        response = "ok";
      }
      return true;
    }

    if (arr[1] == "--unblock")
    {
      auto it = std::lower_bound(Instance->BlockedSubsystems.begin(), Instance->BlockedSubsystems.end(), sid.Name);
      if (it == Instance->BlockedSubsystems.end() || *it != sid.Name)
        response = "error: no such blocked subsystem: " + name;
      else
      {
        Instance->BlockedSubsystems.erase(it);
        response = "ok";
      }
      return true;
    }

    if (arr[1] == "--allow")
    {
      if (HasSubsystem(Instance->AllowedSubsystems, sid.Name))
        response = "error: already allowed: " + name;
      else
      {
        auto it = std::lower_bound(Instance->AllowedSubsystems.begin(), Instance->AllowedSubsystems.end(), sid.Name);
        Instance->AllowedSubsystems.insert(it, sid.Name);
        response = "ok";
      }
      return true;
    }

    if (arr[1] == "--disallow")
    {
      auto it = std::lower_bound(Instance->AllowedSubsystems.begin(), Instance->AllowedSubsystems.end(), sid.Name);
      if (it == Instance->AllowedSubsystems.end() || *it != sid.Name)
        response = "error: no such allowed subsystem: " + name;
      else
      {
        Instance->AllowedSubsystems.erase(it);
        response = "ok";
      }
      return true;
    }

    if (arr[1] == "--check")
    {
      response += std::string("Blocked: ") + (HasSubsystem(Instance->BlockedSubsystems, sid.Name) ? "true" : "false") + "\n";
      response += std::string("Allowed: ") + (HasSubsystem(Instance->AllowedSubsystems, sid.Name) ? "true" : "false") + "\n";

      auto it = findSubsystemLevel(Instance->SubsystemLevels, sid.Name);
      if (it == Instance->SubsystemLevels.end() || it->Subsystem != sid.Name)
        response += "Level: channel default";
      else
        response += "Level: " + GetLevelName(it->FilterLevel);
      return true;
    }
  }

  response += "error: invalid arguments";
  return true;
}
