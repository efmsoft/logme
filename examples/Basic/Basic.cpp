#include <Logme/Logme.h>

static void BasicStream()
{
  LogmeI() << "Hello from stream API";
  LogmeW() << 123;
}

static void BasicPrintf()
{
  LogmeI("Hello %s", "World");
  LogmeE("Error code: %d", -1);
}

static void ChannelAndOverride()
{
  Logme::ID ch{"mychannel"};

  auto channel = Logme::Instance->CreateChannel(ch);
  channel->AddLink(::CH);

  Logme::Override ovr;
  ovr.Remove.Method = true;

  LogmeI(ch) << "Channel stream";
  LogmeI(ch, "Channel printf: %d", 42);

  LogmeI(ch, ovr) << "Override stream";
  LogmeI(ch, ovr, "Override printf: %s", "ok");
}

int main()
{
  BasicStream();
  BasicPrintf();
  ChannelAndOverride();
  return 0;
}
