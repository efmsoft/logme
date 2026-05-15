#include <Logme/ArgumentList.h>
#include <Logme/Logme.h>

using namespace Logme;

static ChannelPtr EnsureVisibleChannel(const ID& id)
{
  auto ch = Logme::Instance->CreateChannel(id);
  ch->AddLink(::CH);
  return ch;
}

static int ProcessItem(int value)
{
  LogmeTPt("ProcessItem entered: %s", ARGS1(value));

  if ((value % 3) == 0)
  {
    LogmeW_TPt("rare branch: %s", ARGS1(value));
  }

  return value * 2;
}

static void RunWorkload(const ChannelPtr& ch)
{
  LogmeTPt(ch, "RunWorkload entered");

  for (int i = 0; i < 6; ++i)
  {
    LogmeTPt(ch, "loop iteration: %s", ARGS1(i));
    ProcessItem(i);
  }
}

static void PrintControlResult(const char* command)
{
  LogmeI("control> %s", command);
  LogmeI("%s", Logme::Instance->Control(command).c_str());
}

int main()
{
  auto ch = EnsureVisibleChannel(ID{ "examples_trace" });

  LogmeI(ch, "Trace Points example");

  RunWorkload(ch);
  PrintControlResult("trace list *:*:*");

  PrintControlResult("trace enable *:*:*");
  RunWorkload(ch);

  PrintControlResult("trace stat *:*:*");
  PrintControlResult("trace disable *:*:*");

  RunWorkload(ch);
  PrintControlResult("trace stat *:*:*");

  return 0;
}
