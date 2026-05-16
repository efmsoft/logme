#include <cstdio>
#include <string>

#include <Logme/Logger.h>

#include "../CommandRegistrar.h"

using namespace Logme;


COMMAND_DESCRIPTOR2("channel", Logger::CommandChannel);

static std::string ToHexUpper(uint64_t value)
{
  char buf[32];
  snprintf(buf, sizeof(buf), LOGME_FMT_U64_HEX_UPPER, (uint64_t)value);
  return std::string(buf);
}

static bool IsOption(const std::string& s)
{
  return s.rfind("--", 0) == 0;
}

static ChannelPtr GetChannelByControlName(const std::string& name)
{
  if (name == "<default>")
    return Logme::Instance->GetExistingChannel(::CH);

  return Logme::Instance->GetExistingChannel(ID{name.c_str()});
}

static std::string NormalizeChannelDisplayName(std::string name)
{
  if (name.empty())
    return "<default>";

  return name;
}

bool Logger::CommandChannel(Logme::StringArray& arr, std::string& response)
{
  if (arr.size() >= 2 && IsOption(arr[1]))
  {
    std::string opt = arr[1];

    if (opt == "--clear-error")
    {
      std::lock_guard guard(Instance->DataLock);
      Instance->ErrorChannel.reset();
      response = "ok";
      return true;
    }

    if (arr.size() < 3)
    {
      response = "error: missing channel name";
      return true;
    }

    std::string name = arr[2];

    std::lock_guard guard(Instance->DataLock);

    if (opt == "--enable" || opt == "--disable")
    {
      auto ch = GetChannelByControlName(name);
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
      auto existing = GetChannelByControlName(name);
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

      auto existing = GetChannelByControlName(name);
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

    if (opt == "--bind")
    {
      if (arr.size() < 4)
      {
        response = "error: missing target channel name";
        return true;
      }

      auto source = GetChannelByControlName(name);
      if (source == nullptr)
      {
        response = "error: no such channel: " + name;
        return true;
      }

      std::string targetName = arr[3];
      auto target = GetChannelByControlName(targetName);
      if (target == nullptr)
      {
        response = "error: no such channel: " + targetName;
        return true;
      }

      source->AddLink(target);
      response = "ok";
      return true;
    }

    if (opt == "--unbind")
    {
      auto existing = GetChannelByControlName(name);
      if (existing == nullptr)
      {
        response = "error: no such channel: " + name;
        return true;
      }

      existing->RemoveLink();
      response = "ok";
      return true;
    }

    if (opt == "--error")
    {
      auto existing = GetChannelByControlName(name);
      if (existing == nullptr)
      {
        response = "error: no such channel: " + name;
        return true;
      }

      if (name == "<default>")
        Instance->SetErrorChannel("");
      else
        Instance->SetErrorChannel(name);

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
    ch = GetChannelByControlName(name);
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

  response += "Channel: " + NormalizeChannelDisplayName(name) + "\n";
  response += "Status: " + std::string(e) + "\n";
  response += "Access count: " + std::to_string(ch->GetAccessCount()) + "\n";
  response += "Logged bytes: " + std::to_string(ch->GetLoggedBytes()) + "\n";

  response += "Flags: 0x" + ToHexUpper(ch->GetFlags().Value) + " " + ch->GetFlags().ToString(" ", true) + "\n";
  response += "Format: " + ch->GetFlags().FormatType() + "\n";
  response += "Level: " + std::string(GetLevelName(ch->GetFilterLevel())) + "\n";

  bool isErrorChannel = false;
  StringPtr errorChannel = Instance->GetErrorChannel();
  if (errorChannel)
  {
    std::string currentName = ch->GetName();
    isErrorChannel = (*errorChannel == currentName);
  }

  response += "Error channel: " + std::string(isErrorChannel ? "Yes" : "No") + "\n";

  ChannelPtr link = ch->GetLinkPtr();
  if (link)
  {
    auto lname = link->GetName();
    if (lname.empty())
      lname = "<default>";

    response += "Linked to: " + lname + "\n";
  }
  else
  {
    response += "Linked to: <none>\n";
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
        response += std::string("  ") + backend->GetType();
        if (backend->Freezed)
          response += " [frozen]";

        std::string details = backend->FormatDetails();
        if (!details.empty())
          response += " " + details;

        response += "\n";
      }
    }
  }
  else
  {
    response += "Backends: 0\n";
  }

  return true;
}
