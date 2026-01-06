#include <Logme/Logme.h>

static void DefaultBehavior()
{
  LogmeI() << "Default output";
}

static void WithoutMethod()
{
  Logme::Override ovr;
  ovr.Remove.Method = true;

  LogmeI(ovr) << "Method removed (stream)";
  LogmeI(ovr, "Method removed (printf): %d", 7);
}

static void WithChannelAndOverride()
{
  Logme::ID ch{"custom"};

  auto channel = Logme::Instance->CreateChannel(ch);
  channel->AddLink(::CH);

  Logme::Override ovr;
  ovr.Remove.Method = true;

  LogmeW(ch, ovr) << "Warning with override";
  LogmeE(ch, ovr, "Error with override: %s", "fail");
}

int main()
{
  DefaultBehavior();
  WithoutMethod();
  WithChannelAndOverride();
  return 0;
}
