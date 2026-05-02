#include <algorithm>
#include <cstring>
#include <string>

#include <Logme/Logger.h>
#include <Logme/SID.h>

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

  if (arr.size() == 1)
  {
    PrintSubsystemList(response, "Blocked subsystems", Instance->BlockedSubsystems);
    PrintSubsystemList(response, "Allowed subsystems", Instance->AllowedSubsystems);
    return true;
  }

  if (arr.size() == 2 && arr[1] == "--clear")
  {
    Instance->BlockedSubsystems.clear();
    Instance->AllowedSubsystems.clear();
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
      response += std::string("Allowed: ") + (HasSubsystem(Instance->AllowedSubsystems, sid.Name) ? "true" : "false");
      return true;
    }
  }

  response += "error: invalid arguments";
  return true;
}
