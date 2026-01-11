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

  // Trim at first '\0'
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

static std::string ModeString(bool block)
{
  return std::string("BlockReportedSubsystems: ") + (block ? "true" : "false") + "\n";
}

bool Logger::CommandSubsystem(Logme::StringArray& arr, std::string& response)
{
  std::lock_guard guard(Instance->DataLock);

  bool block = Instance->BlockReportedSubsystems;

  // Mode switches.
  for (size_t i = 1; i < arr.size(); ++i)
  {
    if (arr[i] == "--block-reported")
      block = true;
    else if (arr[i] == "--unblock-reported")
      block = false;
  }

  bool modify = (arr.size() >= 2 && (arr[1] == "--report" || arr[1] == "--unreport"));

  Instance->SetBlockReportedSubsystems(block);
  if (!modify)
    response += ModeString(Instance->BlockReportedSubsystems);

  if (arr.size() == 1 || (arr.size() == 2 && (arr[1] == "--block-reported" || arr[1] == "--unblock-reported")))
  {
    if (Instance->Subsystems.empty())
    {
      response += "Reported subsystems: none\n";
      return true;
    }

    response += "Reported subsystems:\n";
    for (auto id : Instance->Subsystems)
      response += "  " + SidToString(id) + "\n";
    return true;
  }

  if (arr.size() >= 3 && arr[1] == "--report")
  {
    const std::string& name = arr[2];
    auto sid = SID::Build(name);
    if (sid.Name == 0)
    {
      response += "error: invalid subsystem name";
      return true;
    }

    if (HasSubsystem(Instance->Subsystems, sid.Name))
    {
      response += "error: already reported: " + name;
      return true;
    }

    Instance->ReportSubsystem(sid);
    response = "ok";
    return true;
  }

  if (arr.size() >= 3 && arr[1] == "--unreport")
  {
    const std::string& name = arr[2];
    auto sid = SID::Build(name);
    if (sid.Name == 0)
    {
      response += "error: invalid subsystem name";
      return true;
    }

    if (!HasSubsystem(Instance->Subsystems, sid.Name))
    {
      response += "error: no such reported subsystem: " + name;
      return true;
    }

    Instance->UnreportSubsystem(sid);
    response = "ok";
    return true;
  }

  // subsystem name
  if (arr.size() >= 2 && arr[1].rfind("--", 0) != 0)
  {
    const std::string& name = arr[1];
    auto sid = SID::Build(name);
    if (sid.Name == 0)
    {
      response += "error: invalid subsystem name";
      return true;
    }

    response += std::string("Reported: ") + (HasSubsystem(Instance->Subsystems, sid.Name) ? "true" : "false");
    return true;
  }

  response += "error: invalid arguments";
  return true;
}
