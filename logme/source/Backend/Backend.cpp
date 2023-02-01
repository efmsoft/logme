#include <Logme/Backend/Backend.h>

using namespace Logme;

Backend::Backend(ChannelPtr owner, const char* type)
  : Owner(owner)
  , Type(type)
{
}

Backend::~Backend()
{
}

const char* Backend::GetType() const
{
  return Type;
}
