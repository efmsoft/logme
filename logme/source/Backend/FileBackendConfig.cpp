#include <Logme/Backend/FileBackend.h>
#include <Logme/Logme.h>

#ifdef USE_JSONCPP
#include <json/json.h>
#endif

using namespace Logme;

FileBackendConfig::FileBackendConfig()
  : Append(true)
  , MaxSize(FileBackend::GetMaxSizeDefault())
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
    if (!o["max-size"].isInt())
    {
      LogmeE(CHINT, "\"max-size\" is not an integer value");
      return false;
    }

    MaxSize = o["max-size"].asInt();
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
