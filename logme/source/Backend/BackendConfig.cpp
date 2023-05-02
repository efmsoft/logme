#include <Logme/Backend/Backend.h>

using namespace Logme;

BackendConfig::~BackendConfig()
{
}

bool BackendConfig::Parse(const Json::Value* po)
{
  return true;
}
