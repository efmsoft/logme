#include <Logme/Backend/Backend.h>
#include <Logme/Logme.h>

#ifdef USE_JSONCPP
#include <json/json.h>
#endif

using namespace Logme;

BackendConfig::BackendConfig(const char* type)
  : Type(type)
  , Async(false)
{
}

BackendConfig::~BackendConfig()
{
}

bool BackendConfig::Parse(const Json::Value* po)
{
  (void)po;

#ifdef USE_JSONCPP
  const Json::Value& o = *po;

  if (o.isMember("async"))
  {
    if (!o["async"].isBool())
    {
      LogmeE(CHINT, "\"async\" is not a boolean value");
      return false;
    }

    Async = o["async"].asBool();
  }
#endif

  return true;
}
