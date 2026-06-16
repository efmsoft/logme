#include <Logme/Backend/FileBackend.h>
#include <Logme/Logme.h>

#ifdef USE_JSONCPP
#include <json/json.h>
#endif

#include "../Config/Helper.h"

using namespace Logme;

FileBackendConfig::FileBackendConfig()
  : BackendConfig(FileBackend::TYPE_ID)
  , Append(true)
  , MaxSize(FileBackend::GetMaxSizeDefault())
  , DailyRotation(false)
  , MaxParts(2)
  , GzipCompression(false)
{
  Async = true;
}

FileBackendConfig::~FileBackendConfig()
{
}

bool FileBackendConfig::Parse(const Json::Value* po)
{
  (void)po;

#ifdef USE_JSONCPP
  if (!BackendConfig::Parse(po))
    return false;

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

    if (v == "daily")
      DailyRotation = true;
    else if (v == "" || v == "none" || v == "off" || v == "disabled")
      DailyRotation = false;
    else
    {
      LogmeE(CHINT, "unsupported value of \"rotation\": %s", v.c_str());
      return false;
    }
  }

  if (o.isMember("max-parts"))
  {
    if (!o["max-parts"].isInt())
    {
      LogmeE(CHINT, "\"max-parts\" is not an integer");
      return false;
    }

    MaxParts = o["max-parts"].asInt();
    if (MaxParts < 0)
    {
      LogmeE(CHINT, "\"max-parts\" must not be negative");
      return false;
    }
  }

  if (o.isMember("compression"))
  {
    if (!o["compression"].isString())
    {
      LogmeE(CHINT, "\"compression\" is not a string value");
      return false;
    }

    std::string v = TrimSpaces(o["compression"].asString());
    ToLowerAsciiInplace(v);

    if (v == "gz" || v == "gzip")
      GzipCompression = true;
    else if (v == "" || v == "none" || v == "off" || v == "disabled")
      GzipCompression = false;
    else
    {
      LogmeE(CHINT, "unsupported value of \"compression\": %s", v.c_str());
      return false;
    }
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
