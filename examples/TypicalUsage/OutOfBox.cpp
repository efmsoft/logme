#include <Logme/Logme.h>

static void ProcessRequest(int requestId)
{
  LogmeI("request %d accepted", requestId);
  LogmeW("request %d took longer than expected", requestId);
}

int main()
{
  LogmeI("application started with default logme configuration");

  ProcessRequest(1001);
  LogmeE("example error record");

  return 0;
}
