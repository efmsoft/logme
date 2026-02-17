#include <Logme/Backend/BufferBackend.h>
#include <Logme/Logme.h>

#ifdef USE_JSONCPP
#include <json/json.h>
#endif

#include "../Config/Helper.h"

using namespace Logme;

static NAMED_VALUE PolicyValues[] =
{
  {"", (int)BufferBackendPolicy::STOP_APPENDING},

  {"stop", (int)BufferBackendPolicy::STOP_APPENDING},
  {"stop-appending", (int)BufferBackendPolicy::STOP_APPENDING},
  {"stop_appending", (int)BufferBackendPolicy::STOP_APPENDING},

  {"delete", (int)BufferBackendPolicy::DELETE_OLDEST},
  {"delete-oldest", (int)BufferBackendPolicy::DELETE_OLDEST},
  {"delete_oldest", (int)BufferBackendPolicy::DELETE_OLDEST},

  {nullptr, 0}
};

BufferBackendConfig::BufferBackendConfig()
  : BackendConfig(BufferBackend::TYPE_ID)
  , MaxSize(MAX_SIZE_DEFAULT)
  , Policy(BufferBackendPolicy::STOP_APPENDING)
{
}

bool BufferBackendConfig::Parse(const Json::Value* po)
{
  (void)po;

#ifdef USE_JSONCPP
  const Json::Value& o = *po;

  if (o.isMember("policy"))
  {
    if (!o["policy"].isString())
    {
      LogmeE(CHINT, "\"policy\" is not a string value");
      return false;
    }

    int v = 0;
    if (Name2Value(o["policy"].asString(), false, PolicyValues, v))
    {
      Policy = (BufferBackendPolicy)v;
    }
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
#endif

  return true;
}
