#include <Logme/Logme.h>
#include "Helper.h"

using namespace Logme;

static void SetConfigurationError(
  std::string* error
  , const std::string& text
)
{
  if (error == nullptr)
    return;

  *error = text;
}

bool Logger::LoadConfigurationFile(
  const std::wstring& config_file
  , const std::string& section
  , std::string* error
)
{
  SetConfigurationError(error, std::string());

  try
  {
    std::string data = ReadFile(config_file);
    return LoadConfiguration(data, section, error);
  }
  catch (std::exception const& e)
  {
    SetConfigurationError(error, e.what());
  }

  return false;
}

bool Logger::LoadConfigurationFile(
  const std::string& config_file
  , const std::string& section
  , std::string* error
)
{
  SetConfigurationError(error, std::string());

  try
  {
    std::string data = ReadFile(config_file);
    return LoadConfiguration(data, section, error);
  }
  catch (std::exception const& e)
  {
    SetConfigurationError(error, e.what());
  }

  return false;
}

bool Logger::LoadConfiguration(
  const std::string& config_data
  , const std::string& section
  , std::string* error
)
{
  SetConfigurationError(error, std::string());

  (void)config_data;
  (void)section;

#ifdef USE_JSONCPP
  Json::Value root;
  Json::Reader reader;
  if (!reader.parse(config_data, root))
  {
    SetConfigurationError(error, reader.getFormattedErrorMessages());
    return false;
  }

  Json::Value config;
  if (!GetConfigSection(root, section, config))
  {
    SetConfigurationError(error, "configuration section was not found");
    return false;
  }

  ControlConfig cc{};
  if (!ParseControlConfig(config, cc))
  {
    SetConfigurationError(error, "control configuration parsing failed");
    return false;
  }

  OutputFlagsMap flags;
  if (!ParseFlags(config, flags))
  {
    SetConfigurationError(error, "output flags parsing failed");
    return false;
  }

  ChannelConfigArray arr;
  if (!ParseChannels(config, flags, arr))
  {
    SetConfigurationError(error, "channel configuration parsing failed");
    return false;
  }

  bool blockReported = true;
  std::list<std::string> subsystems;
  std::list<std::string> blockedSubsystems;
  std::list<std::string> allowedSubsystems;
  if (!ParseSubsystems(
        config
        , blockReported
        , subsystems
        , blockedSubsystems
        , allowedSubsystems
      ))
  {
    SetConfigurationError(error, "subsystem configuration parsing failed");
    return false;
  }

  HomeDirectoryConfig hdc;
  if (!ParseHomeDirectoryConfig(config, hdc))
  {
    SetConfigurationError(error, "home directory configuration parsing failed");
    return false;
  }

  bool rc = CreateChannels(arr);
  ReplaceChannels(arr);

  ClearSubsystemFilters();
  BlockReportedSubsystems = blockReported;

  for (auto& s : blockedSubsystems)
    AddBlockedSubsystem(SID::Build(s));

  for (auto& s : allowedSubsystems)
    AddAllowedSubsystem(SID::Build(s));

  for (auto& s : subsystems)
  {
    if (blockReported)
      AddBlockedSubsystem(SID::Build(s));
    else
      AddAllowedSubsystem(SID::Build(s));
  }

  SetHomeDirectory(hdc.HomeDirectory);
  HomeDirectoryWatchDog.SetMaximalSize(hdc.MaximalSize);
  HomeDirectoryWatchDog.SetPeriodicity(hdc.CheckPeriodicity);
  HomeDirectoryWatchDog.SetExtensions(hdc.Extensions);
  
  if (hdc.EnableWatchDog)
    HomeDirectoryWatchDog.Run();

  bool exitcode = StartControlServer(cc) && rc;
  if (exitcode == false)
  {
    SetConfigurationError(error, "configuration was parsed but could not be applied");
    LogmeE(CHINT, "configuration was parsed but could not be applied");
  }
  return exitcode;
#else
  SetConfigurationError(error, "JSON configuration support is not enabled");
  return false;
#endif
}
