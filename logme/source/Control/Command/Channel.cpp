#include <cstdio>
#include <string>

#include <Logme/Logger.h>

#include "../CommandRegistrar.h"

using namespace Logme;

COMMAND_DESCRIPTOR2("channel", Logger::CommandChannel);

static std::string ToHexUpper(uint64_t value)
{
  char buf[32];
  snprintf(buf, sizeof(buf), "%llX", (unsigned long long)value);
  return std::string(buf);
}

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
