#include <cstdio>
#include <string>

#include <Logme/Logger.h>

#include "../CommandRegistrar.h"

using namespace Logme;

#ifdef _WIN32
#  define LLX "%llX"
#else
#  define LLX "%lX"
#endif

COMMAND_DESCRIPTOR2("channel", Logger::CommandChannel);

static std::string ToHexUpper(uint64_t value)
{
  char buf[32];
  snprintf(buf, sizeof(buf), LLX, (uint64_t)value);
  return std::string(buf);
}

static bool IsOption(const std::string& s)
{
  return s.rfind("--", 0) == 0;
}

bool Logger::CommandChannel(Logme::StringArray& arr, std::string& response)
{
  if (arr.size() >= 2 && IsOption(arr[1]))
  {
    if (arr.size() < 3)
    {
      response = "error: missing channel name";
      return true;
    }

    std::string opt = arr[1];
    std::string name = arr[2];

    std::lock_guard guard(Instance->DataLock);

    if (opt == "--enable" || opt == "--disable")
    {
      auto ch = Instance->GetExistingChannel(ID{name.c_str()});
      if (ch == nullptr)
      {
        response = "error: no such channel: " + name;
        return true;
      }

      ch->SetEnabled(opt == "--enable");
      response = "ok";
      return true;
    }

    if (opt == "--create")
    {
      auto existing = Instance->GetExistingChannel(ID{name.c_str()});
      if (existing != nullptr)
      {
        response = "error: channel already exists: " + name;
        return true;
      }

      Instance->CreateChannel(ID{name.c_str()});
      response = "ok";
      return true;
    }

    if (opt == "--delete")
    {
      if (name.empty())
      {
        response = "error: invalid channel name";
        return true;
      }

      if (name == "<default>")
      {
        response = "error: cannot delete default channel";
        return true;
      }

      auto existing = Instance->GetExistingChannel(ID{name.c_str()});
      if (existing == nullptr)
      {
        response = "error: no such channel: " + name;
        return true;
      }

      // Default channel is represented by empty name.
      if (existing->GetName().empty())
      {
        response = "error: cannot delete default channel";
        return true;
      }

      Instance->DeleteChannel(ID{name.c_str()});
      response = "ok";
      return true;
    }

    response = "error: unknown option: " + opt;
    return true;
  }

  ChannelPtr ch;
  std::string name;
  if (arr.size() > 1)
  {
    name = arr[1];
    ch = Instance->GetExistingChannel(ID{name.c_str()});
  }
  else
  {
    name = "<default>";
    ch = Instance->GetExistingChannel(::CH);
  }

  if (ch == nullptr)
  {
    response = "error: no such channel: " + name;
    return true;
  }

  const char* e = ch->GetEnabled() ? "Enabled" : "Disabled";
  const char* action = ch->GetEnabled() ? " <a href='#'>Disable</a>" : " <a href='#'>Enable</a>";

  response += "Channel: " + name + "\n";
  response += "Status: " + std::string(e) + action + "\n";
  response += "Access count: " + std::to_string(ch->GetAccessCount()) + "\n";

  response += "Flags: 0x" + ToHexUpper(ch->GetFlags().Value) + " " + ch->GetFlags().ToString(" ", true) + "\n";
  response += "Level: " + std::string(GetLevelName(ch->GetFilterLevel())) + "\n";

  ChannelPtr link = ch->GetLinkPtr();
  if (link)
  {
    auto lname = link->GetName();
    if (lname.empty())
      lname = "<default>";

    response += "Linked to: " + lname + "\n";
  }

  auto bc = ch->NumberOfBackends();
  if (bc)
  {
    response += "Backends: " + std::to_string(bc) + "\n";
    for (size_t i = 0; i < bc; ++i)
    {
      auto backend = ch->GetBackend(i);
      if (backend)
      {
        response += std::string("  ") + backend->GetType() + " " + backend->FormatDetails() + "\n";
      }
    }
  }

  return true;
}
