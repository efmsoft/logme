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

void DebugBackend::Display(Context& context, const char* line)
{
#ifdef _WIN32
  int nc;
  const char* buffer = context.Apply(Owner, Owner->GetFlags(), line, nc);

  OutputDebugStringA(buffer);
#endif
}
