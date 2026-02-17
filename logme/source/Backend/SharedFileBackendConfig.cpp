#include <Logme/Backend/FileBackend.h>
#include <Logme/Backend/SharedFileBackend.h>
#include <Logme/Logme.h>

#ifdef USE_JSONCPP
#include <json/json.h>
#endif

#include "../Config/Helper.h"

using namespace Logme;

SharedFileBackendConfig::SharedFileBackendConfig()
  : BackendConfig(SharedFileBackend::TYPE_ID)
  , MaxSize(FileBackend::GetMaxSizeDefault())
  , Timeout(10)
{
}

SharedFileBackendConfig::~SharedFileBackendConfig()
{
}

bool SharedFileBackendConfig::Parse(const Json::Value* po)
{
  (void)po;

#ifdef USE_JSONCPP
  const Json::Value& o = *po;

  if (o.isMember("max-size"))
  {
    if (!o["max-size"].isInt() && !o["max-size"].isString())
    {
      LogmeE(CHINT, "\"max-size\" is not an integer or a string value");
      return false;
    }

    MaxSize = GetByteSize(o, "max-size", MaxSize);
  }

  if (o.isMember("timeout"))
  {
    if (!o["timeout"].isInt() && !o["timeout"].isString())
    {
      LogmeE(CHINT, "\"timeout\" is not an integer or a string value");
      return false;
    }

    Timeout = GetInterval(o, "timeout", MaxSize);
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
