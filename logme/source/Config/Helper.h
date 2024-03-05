#pragma once

#include <filesystem>
#include <fstream>
#include <string>

#include <Logme/Logme.h>

#ifdef USE_JSONCPP

#include <json/json.h>

bool ParseControlConfig(const Json::Value& root, Logme::ControlConfig& cc);
bool ParseFlags(const Json::Value& root, Logme::OutputFlagsMap& m);
bool ParseChannels(const Json::Value& root, Logme::OutputFlagsMap& m, Logme::ChannelConfigArray& arr);
bool GetConfigSection(const Json::Value& root, const std::string& section, Json::Value& config);

#endif

namespace fs = std::filesystem;

bool Name2Value(const std::string& name, bool caseSensitive, const Logme::NAMED_VALUE* pv, int& v);
std::string ReadFile(const fs::path& path);
bool CheckBuild(const std::string& build, bool& ok);
bool CheckPlatform(const std::string& build, bool& ok);