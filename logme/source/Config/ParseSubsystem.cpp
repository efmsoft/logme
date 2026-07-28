#include <Logme/Logme.h>

#include "Helper.h"

using namespace Logme;

#ifdef USE_JSONCPP
#include <json/json.h>

static bool ParseSubsystemList(
  const Json::Value& root
  , const char* name
  , std::list<std::string>& arr
)
{
  if (!root.isMember(name))
    return true;

  auto& l = root[name];

  if (!l.isArray())
  {
    LogmeE(CHINT, "\"subsystems[\"%s\"]\" is not an array", name);
    return false;
  }

  for (Json::Value::ArrayIndex i = 0; i < l.size(); ++i)
  {
    auto& o = l[i];

    if (!o.isString())
    {
      LogmeE(CHINT, "\"subsystems[\"%s\"][%i]\" is not a string", name, i);
      return false;
    }

    arr.push_back(o.asString());
  }

  return true;
}


static bool ParseSubsystemLevels(
  const Json::Value& root
  , std::list<std::pair<std::string, Level>>& levels
)
{
  if (!root.isMember("levels"))
    return true;

  const auto& configuredLevels = root["levels"];
  if (!configuredLevels.isObject())
  {
    LogmeE(CHINT, "\"subsystems[\"levels\"]\" is not an object");
    return false;
  }

  for (const std::string& name : configuredLevels.getMemberNames())
  {
    if (name.empty())
    {
      LogmeE(CHINT, "\"subsystems[\"levels\"]\" contains an empty subsystem name");
      return false;
    }

    const auto& value = configuredLevels[name];
    if (!value.isString())
    {
      LogmeE(
        CHINT
        , "\"subsystems[\"levels\"][\"%s\"]\" is not a string"
        , name.c_str()
      );
      return false;
    }

    int parsedLevel = 0;
    if (!LevelFromName(value.asString(), parsedLevel))
    {
      LogmeE(
        CHINT
        , "\"subsystems[\"levels\"][\"%s\"]\" value is not supported"
        , name.c_str()
      );
      return false;
    }

    levels.emplace_back(name, static_cast<Level>(parsedLevel));
  }

  return true;
}

bool ParseSubsystems(
  const Json::Value& root
  , bool& blockListed
  , std::list<std::string>& arr
  , std::list<std::string>& blocked
  , std::list<std::string>& allowed
  , std::list<std::pair<std::string, Level>>& levels
)
{
  blockListed = true;

  if (!root.isMember("subsystems"))
    return true;

  auto& c = root["subsystems"];

  if (!c.isObject())
  {
    LogmeE(CHINT, "\"subsystems\" is not an object");
    return false;
  }

  if (c.isMember("block-listed"))
  {
    if (c["block-listed"].isBool() == false)
    {
      LogmeE(CHINT, "\"subsystems[\"block-listed\"]\" is not a boolean");
      return false;
    }

    blockListed = c["block-listed"].asBool();
  }

  if (!ParseSubsystemList(c, "blocked", blocked))
    return false;

  if (!ParseSubsystemList(c, "allowed", allowed))
    return false;

  if (!ParseSubsystemList(c, "list", arr))
    return false;

  if (!ParseSubsystemLevels(c, levels))
    return false;

  return true;
}

#endif
