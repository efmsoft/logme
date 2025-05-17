#pragma once

#include <filesystem>
#include <fstream>
#include <list>
#include <stdint.h>
#include <string>

#include <Logme/Logme.h>

#ifdef USE_JSONCPP

#include <json/json.h>

bool ParseControlConfig(const Json::Value& root, Logme::ControlConfig& cc);
bool ParseFlags(const Json::Value& root, Logme::OutputFlagsMap& m);
bool ParseChannels(const Json::Value& root, Logme::OutputFlagsMap& m, Logme::ChannelConfigArray& arr);
bool GetConfigSection(const Json::Value& root, const std::string& section, Json::Value& config);
bool ParseSubsystems(const Json::Value& root, bool& blockListed, std::list<std::string>& arr);

struct Suffix
{
  const char* Name;
  int Multiplier;
};

uint64_t SmartValue(
  const Json::Value& root
  , const char* option
  , uint64_t def
  , Suffix* suffixes
  , size_t n
);

struct HomeDirectoryConfig
{
  std::string HomeDirectory;

  bool EnableWatchDog;
  uint64_t MaximalSize;
  int CheckPeriodicity;
  std::vector<std::string> Extensions;

  HomeDirectoryConfig()
    : EnableWatchDog(false)
    , MaximalSize(0)
    , CheckPeriodicity(0)
  {
  }
};
bool ParseHomeDirectoryConfig(const Json::Value& root, HomeDirectoryConfig& hdc);

#endif

namespace fs = std::filesystem;

bool Name2Value(const std::string& name, bool caseSensitive, const Logme::NAMED_VALUE* pv, int& v);
std::string ReadFile(const fs::path& path);
bool CheckBuild(const std::string& build, bool& ok);
bool CheckPlatform(const std::string& build, bool& ok);