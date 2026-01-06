#include <Logme/Logme.h>
#include <Logme/ArgumentList.h>
#include <Logme/Procedure.h>

#include <thread>
#include <vector>

using namespace Logme;

static ChannelPtr EnsureVisibleChannel(const ID& id)
{
  auto ch = Logme::Instance->CreateChannel(id);
  ch->AddLink(::CH);
  return ch;
}

struct Point
{
  int X;
  int Y;
};

static std::ostream& operator<<(std::ostream& os, const Point& p)
{
  os << "Point(" << p.X << "," << p.Y << ")";
  return os;
}

template<> std::string FormatValue<Point>(const Point& value)
{
  return "Point(" + std::to_string(value.X) + ", " + std::to_string(value.Y) + ")";
}

static int Sum(int a, int b)
{
  int r = a + b;
  LogmeP(r, ARGS2(a, b));
  return r;
}

static Point MakePoint(int x, int y)
{
  Point r{ x, y };
  LogmeP(r, ARGS2(x, y));
  return r;
}

static void DoWork(const char* name, int count)
{
  LogmePV(ARGS2(name, count));
  for (int i = 0; i < count; i++)
  {
    LogmeD() << "tick=" << i;
  }
}

static void LibraryFunctionThatDoesNotKnowTheChannel()
{
  LogmeW() << "This message goes to the thread channel if it is set.";
}

int main()
{
  auto ch = EnsureVisibleChannel(ID{ "examples_procedure" });

  LogmeI(ch) << "Procedure macros and custom printers example";

  // 1) Procedure macros: enter/leave with return value and named arguments.
  int sum = Sum(10, 32);
  LogmeI(ch, "Sum returned %d", sum);

  // 2) Custom printer: just provide operator<< for your type.
  Point p = MakePoint(7, 9);
  LogmeI(ch) << "MakePoint returned " << p;

  // 3) Void procedure (enter/leave + arguments).
  DoWork("job", 3);

  // 4) Thread channel: route library logs without passing ChannelPtr.
  {
    LogmeThreadChannel(ch->GetID());
    LibraryFunctionThatDoesNotKnowTheChannel();
  }

  return 0;
}
