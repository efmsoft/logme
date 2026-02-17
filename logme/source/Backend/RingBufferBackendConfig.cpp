#include <Logme/Backend/RingBufferBackend.h>
#include <Logme/Logme.h>

#ifdef USE_JSONCPP
#include <json/json.h>
#endif

#include "../Config/Helper.h"

using namespace Logme;

RingBufferBackendConfig::RingBufferBackendConfig()
  : BackendConfig(RingBufferBackend::TYPE_ID)
  , MaxItems(MAX_ITEMS_DEFAULT)
{
}

bool RingBufferBackendConfig::Parse(const Json::Value* po)
{
  (void)po;

#ifdef USE_JSONCPP
  const Json::Value& o = *po;

  if (o.isMember("max-items"))
  {
    if (!o["max-items"].isInt())
    {
      LogmeE(CHINT, "\"max-items\" is not an integer");
      return false;
    }

    int v = o["max-items"].asInt();
    if (v < 1)
    {
      LogmeE(CHINT, "\"max-items\" must be greater than 0");
      return false;
    }

    MaxItems = v;
  }
#endif

  return true;
}
