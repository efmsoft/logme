#include <Logme/Backend/CallbackBackend.h>

using namespace Logme;

CallbackBackendConfig::CallbackBackendConfig()
  : BackendConfig(CallbackBackend::TYPE_ID)
{
}

bool CallbackBackendConfig::Parse(const Json::Value* po)
{
  return BackendConfig::Parse(po);
}
