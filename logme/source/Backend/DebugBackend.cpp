#include <Logme/Backend/DebugBackend.h>
#include <Logme/Channel.h>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace Logme;

DebugBackend::DebugBackend(ChannelPtr owner)
  : Backend(owner, TYPE_ID)
{
}

void DebugBackend::Display(Context& context)
{
  (void)context;
  
#ifdef _WIN32
  int nc;
  const char* buffer = context.Apply(Owner, Owner->GetFlags(), nc);

  OutputDebugStringA(buffer);
#endif
}
