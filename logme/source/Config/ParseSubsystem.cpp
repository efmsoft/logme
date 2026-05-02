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

bool ParseSubsystems(
  const Json::Value& root
  , bool& blockListed
  , std::list<std::string>& arr
  , std::list<std::string>& blocked
  , std::list<std::string>& allowed
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

  return true;
}

#endif
