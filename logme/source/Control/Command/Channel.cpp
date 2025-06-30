#include <format>

#include <Logme/Logger.h>

#include "../CommandRegistrar.h"

using namespace Logme;

COMMAND_DESCRIPTOR2("channel", Logger::CommandChannel);

bool Logger::CommandChannel(Logme::StringArray& arr, std::string& response)
{
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
    response = "no such channel: " + name;
    return true;
  }

  auto e = ch->GetEnabled() ? "Enabled" : "Disabled";
  auto action = ch->GetEnabled() ? " <a href='#'>Disable</a>" : " <a href='#'>Enable</a>";
  
  response += std::format("Channel: {}\n", name);
  response += std::format("Status: {}{}\n", std::string(e), action);
  response += std::format("Access count: {}\n", ch->GetAccessCount());

  response += std::format("Flags: 0x{:X} {}\n", ch->GetFlags().Value, ch->GetFlags().ToString(" ", true));
  response += std::format("Level: {}\n", GetLevelName(ch->GetFilterLevel()));

  ChannelPtr link = ch->GetLinkPtr();
  if (link)
  {
    auto lname = link->GetName();
    if (lname.empty())
      lname = "<default>";

    response += std::format("Linked to: {}\n", lname);
  }

  auto bc = ch->NumberOfBackends();
  if (bc)
  {
    response += std::format("Backends: {}\n", bc);
    for (size_t i = 0; i < bc; ++i)
    {
      auto backend = ch->GetBackend(i);
      if (backend)
      {
        response += std::format("  {} {}\n", backend->GetType(), backend->FormatDetails());
      }
    }
  }

  return true;
}

