#include <Logme/Logme.h>

#include "Helper.h"

using namespace Logme;

#ifdef USE_JSONCPP
#include <json/json.h>

bool ParseSubsystems(
  const Json::Value& root
  , bool& blockListed
  , std::list<std::string>& arr
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

  if (!c.isMember("list"))
    return true;

  auto& l = c["list"];

  if (!l.isArray())
  {
    LogmeE(CHINT, "\"subsystems[\"list\"]\" is not an array");
    return false;
  }

  bool ok = true;
  for (Json::Value::ArrayIndex i = 0; i < l.size(); ++i)
  {
    auto& o = l[i];

    if (!o.isString())
    {
      LogmeE(CHINT, "\"subsystems[\"list\"][%i]\" is not a string", i);
      return false;
    }

    arr.push_back(o.asString());
  }

  return true;
}

#endif
