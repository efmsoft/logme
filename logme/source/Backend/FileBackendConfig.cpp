#include <Logme/Backend/FileBackend.h>
#include <Logme/Logme.h>

#ifdef USE_JSONCPP
#include <json/json.h>
#endif

#include "../Config/Helper.h"

using namespace Logme;

FileBackendConfig::FileBackendConfig()
  : Append(true)
  , MaxSize(FileBackend::GetMaxSizeDefault())
  , DailyRotation(false)
  , MaxParts(2)
{
}

FileBackendConfig::~FileBackendConfig()
{
}

bool FileBackendConfig::Parse(const Json::Value* po)
{
#ifdef USE_JSONCPP
  const Json::Value& o = *po;

  if (o.isMember("append"))
  {
    if (!o["append"].isBool())
    {
      LogmeE(CHINT, "\"append\" is not a boolean value");
      return false;
    }

    Append = o["append"].asBool();
  }

  if (o.isMember("max-size"))
  {
    if (!o["max-size"].isInt() && !o["max-size"].isString())
    {
      LogmeE(CHINT, "\"max-size\" is not an integer or a string value");
      return false;
    }

    MaxSize = GetByteSize(o, "max-size", MaxSize);
  }

  if (o.isMember("rotation"))
  {
    if (!o["rotation"].isString())
    {
      LogmeE(CHINT, "\"rotation\" is not a string value");
      return false;
    }

    std::string v = TrimSpaces(o["rotation"].asString());
    ToLowerAsciiInplace(v);

    DailyRotation = v == "daily";
  }

  if (o.isMember("max-parts"))
  {
    if (!o["max-parts"].isInt())
    {
      LogmeE(CHINT, "\"max-parts\" is not an integer");
      return false;
    }

    MaxParts = o["max-parts"].asInt();
  }

  if (!o.isMember("file"))
  {
    LogmeE(CHINT, "\"file\" is not specified");
    return false;
  }

  if (!o["file"].isString())
  {
    LogmeE(CHINT, "\"file\" is not a string");
    return false;
  }

  Filename = o["file"].asString();
#endif

  return true;
}
