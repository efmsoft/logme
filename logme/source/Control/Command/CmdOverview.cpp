#include <cstdio>
#include <map>
#include <string>

#include <Logme/Backend/Backend.h>
#include <Logme/Channel.h>
#include <Logme/Logger.h>

#include "../CommandRegistrar.h"

using namespace Logme;

COMMAND_DESCRIPTOR2("overview", Logger::CommandOverview);

static std::string FormatBool(bool value)
{
  return value ? "Enabled" : "Disabled";
}

static std::string FormatErrorChannel(const StringPtr& channel)
{
  if (!channel)
    return "<none>";

  if (channel->empty())
    return "<default>";

  return *channel;
}

static std::string FormatObfuscationKey(const ObfKey* key)
{
  if (!key)
    return "";

  std::string s;
  s.reserve(LOGOBF_KEY_BYTES * 2);

  for (size_t i = 0; i < LOGOBF_KEY_BYTES; ++i)
  {
    char buf[4];
    snprintf(buf, sizeof(buf), "%02X", (unsigned)key->Bytes[i]);
    s += buf;
  }

  return s;
}

static void CollectChannelStats(
  const ChannelPtr& channel
  , uint64_t& accessCount
  , uint64_t& loggedBytes
  , std::map<std::string, size_t>& backendTypes
  , size_t& backendMemory
)
{
  if (!channel)
    return;

  accessCount += channel->GetAccessCount();
  loggedBytes += channel->GetLoggedBytes();

  auto backendCount = channel->NumberOfBackends();
  for (size_t i = 0; i < backendCount; ++i)
  {
    auto backend = channel->GetBackend(i);
    if (!backend)
      continue;

    backendTypes[backend->GetType()]++;
    backendMemory += backend->GetMemoryUsage();
  }
}

bool Logger::CommandOverview(Logme::StringArray& arr, std::string& response)
{
  (void)arr;

  std::lock_guard guard(Instance->DataLock);

  uint64_t accessCount = 0;
  uint64_t loggedBytes = 0;
  std::map<std::string, size_t> backendTypes;
  size_t backendMemory = 0;

  CollectChannelStats(
    Instance->Default
    , accessCount
    , loggedBytes
    , backendTypes
    , backendMemory
  );

  for (auto& item : Instance->Channels)
  {
    CollectChannelStats(
      item.second
      , accessCount
      , loggedBytes
      , backendTypes
      , backendMemory
    );
  }

  const ObfKey* key = Instance->GetObfuscationKey();

  response += "Home directory: " + Instance->HomeDirectory + "\n";
  response += "Channels: " + std::to_string(Instance->Channels.size() + 1) + "\n";
  response += "Auto delete channels: " + std::to_string(Instance->ToDelete.size() + Instance->NumDeleting) + "\n";
  response += "VT mode requested: " + FormatBool(Instance->EnableVTMode) + "\n";
  response += "Obfuscation: " + FormatBool(key != nullptr) + "\n";

  if (key)
    response += "Obfuscation key: " + FormatObfuscationKey(key) + "\n";

  response += "Error channel: " + FormatErrorChannel(Instance->ErrorChannel) + "\n";
  response += "Log write calls: " + std::to_string(accessCount) + "\n";
  response += "Logged bytes: " + std::to_string(loggedBytes) + "\n";

  if (backendMemory != 0)
    response += "Backend memory: " + std::to_string(backendMemory) + "\n";

  if (backendTypes.empty())
  {
    response += "Backends: none\n";
  }
  else
  {
    response += "Backends:\n";
    for (auto& item : backendTypes)
      response += "  " + item.first + ": " + std::to_string(item.second) + "\n";
  }

  return true;
}
