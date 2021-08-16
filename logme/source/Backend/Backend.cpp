#include <Logme/Backend/Backend.h>

using namespace Logme;

Backend::Backend(ChannelPtr owner)
  : Owner(owner)
{
}

Backend::~Backend()
{
}
