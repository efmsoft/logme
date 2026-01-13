#include <Logme/Logme.h>
#include "Helper.h"

using namespace Logme;

bool Logger::LoadConfigurationFile(
  const std::wstring& config_file
  , const std::string& section
)
{
  try
  {
    std::string data = ReadFile(config_file);
    return LoadConfiguration(data, section);
  }
  catch (std::filesystem::filesystem_error const& e)
  {
    (void)e;
  }

  return false;
}

bool Logger::LoadConfigurationFile(
  const std::string& config_file
  , const std::string& section
)
{
  try
  {
    std::string data = ReadFile(config_file);
    return LoadConfiguration(data, section);
  }
  catch (std::filesystem::filesystem_error const& e)
  {
    (void)e;
  }

  return false;
}

bool Logger::LoadConfiguration(
  const std::string& config_data
  , const std::string& section
)
{
#ifdef USE_JSONCPP
  Json::Value root;
  Json::Reader reader;
  if (!reader.parse(config_data, root))
    return false;

  Json::Value config;
  if (!GetConfigSection(root, section, config))
    return false;

  ControlConfig cc{};
  if (!ParseControlConfig(config, cc))
    return false;

  OutputFlagsMap flags;
  if (!ParseFlags(config, flags))
    return false;

  ChannelConfigArray arr;
  if (!ParseChannels(config, flags, arr))
    return false;

  bool blockReported = true;
  std::list<std::string> subsystems;
  if (!ParseSubsystems(config, blockReported, subsystems))
    return false;

  HomeDirectoryConfig hdc;
  if (!ParseHomeDirectoryConfig(config, hdc))
    return false;

  bool rc = CreateChannels(arr);
  ReplaceChannels(arr);

  Subsystems.clear();
  for (auto& s : subsystems)
    ReportSubsystem(SID::Build(s));

  BlockReportedSubsystems = blockReported;

  SetHomeDirectory(hdc.HomeDirectory);
  HomeDirectoryWatchDog.SetMaximalSize(hdc.MaximalSize);
  HomeDirectoryWatchDog.SetPeriodicity(hdc.CheckPeriodicity);
  HomeDirectoryWatchDog.SetExtensions(hdc.Extensions);
  
  if (hdc.EnableWatchDog)
    HomeDirectoryWatchDog.Run();

  bool exitcode = StartControlServer(cc) && rc;
  if (exitcode == false)
  {
    LogmeE(CHINT, "LoadConfiguration() failed");
  }
  return exitcode;
#else
  return false;
#endif
}
