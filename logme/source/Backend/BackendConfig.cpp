#include <Logme/Backend/Backend.h>

using namespace Logme;

BackendConfig::BackendConfig(const char* type)
  : Type(type)
{
}

BackendConfig::~BackendConfig()
{
}

bool BackendConfig::Parse(const Json::Value* po)
{
  (void)po;
  return true;
}
