#include <Logme/Backend/WindowsEventLogBackend.h>
#include <Logme/Logme.h>

#ifdef USE_JSONCPP
#include <json/json.h>
#endif

using namespace Logme;

WindowsEventLogBackendConfig::WindowsEventLogBackendConfig()
  : BackendConfig(WindowsEventLogBackend::TYPE_ID)
  , EventId(1000)
  , Category(0)
{
}

bool WindowsEventLogBackendConfig::Parse(const Json::Value* po)
{
  (void)po;

#ifdef USE_JSONCPP
  if (!BackendConfig::Parse(po))
    return false;

  const Json::Value& o = *po;

  if (o.isMember("source"))
  {
    if (!o["source"].isString())
    {
      LogmeE(CHINT, "\"source\" is not a string");
      return false;
    }

    Source = o["source"].asString();
  }

  if (o.isMember("event-id"))
  {
    if (!o["event-id"].isUInt())
    {
      LogmeE(CHINT, "\"event-id\" is not an unsigned integer");
      return false;
    }

    EventId = o["event-id"].asUInt();
  }

  if (o.isMember("category"))
  {
    if (!o["category"].isUInt())
    {
      LogmeE(CHINT, "\"category\" is not an unsigned integer");
      return false;
    }

    unsigned int category = o["category"].asUInt();
    if (category > UINT16_MAX)
    {
      LogmeE(CHINT, "\"category\" value is too large");
      return false;
    }

    Category = static_cast<uint16_t>(category);
  }
#endif

  return true;
}
