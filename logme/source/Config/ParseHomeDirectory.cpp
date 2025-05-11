#include <Logme/Logme.h>
#include <Logme/Template.h>

#include "Helper.h"

using namespace Logme;

#ifdef USE_JSONCPP
#include <json/json.h>

bool ParseHomeDirectoryConfig(
  const Json::Value& root
  , HomeDirectoryConfig& hdc
)
{
  if (!root.isMember("home-directory"))
    return true;

  auto& c = root["home-directory"];

  if (!c.isObject())
  {
    LogmeE(CHINT, "\"home-directory\" is not an object");
    return false;
  }

  if (c.isMember("path"))
  {
    if (c["path"].isString() == false)
    {
      LogmeE(CHINT, "\"home-directory[\"path\"]\" is not a string");
      return false;
    }

    std::string str = c["path"].asString();

    ProcessTemplateParam param;
    hdc.HomeDirectory = ProcessTemplate(str.c_str(), param);
  }

  if (!c.isMember("watch-dog"))
    return true;

  auto& dog = c["watch-dog"];

  if (!dog.isObject())
  {
    LogmeE(CHINT, "\"home-directory[\"watch-dog\"]\" is not an object");
    return false;
  }

  if (dog.isMember("enable"))
  {
    if (dog["enable"].isBool() == false)
    {
      LogmeE(CHINT, "\"watch-dog[\"enable\"]\" is not a boolean");
      return false;
    }

    hdc.EnableWatchDog = dog["enable"].asBool();
  }

  if (dog.isMember("max-size"))
  {
    if (dog["max-size"].isString() == false && dog["max-size"].isInt() == false)
    {
      LogmeE(CHINT, "\"watch-dog[\"max-size\"]\" is not a string or integer");
      return false;
    }

    hdc.MaximalSize = GetByteSize(dog, "max-size", 0);
  }

  if (dog.isMember("check-periodicity"))
  {
    if (dog["check-periodicity"].isString() == false && dog["check-periodicity"].isInt() == false)
    {
      LogmeE(CHINT, "\"watch-dog[\"check-periodicity\"]\" is not a string or integer");
      return false;
    }

    hdc.CheckPeriodicity = GetInterval(dog, "check-periodicity", 0);
  }

  if (!dog.isMember("file-extension"))
    return true;

  if (dog["file-extension"].isArray() == false)
  {
    LogmeE(CHINT, "\"watch-dog[\"file-extension\"]\" is not an array");
    return false;
  }

  auto& e = dog["file-extension"];
  for (Json::Value::ArrayIndex i = 0; i < e.size(); ++i)
  {
    auto& oe = e[i];
    if (oe.isString() == false)
    {
      LogmeE(CHINT, "\"watch-dog[\"file-extension\"][%i]\" is not a string", i);
      return false;
    }

    hdc.Extensions.push_back(oe.asString());
  }

  return true;
}

#endif
