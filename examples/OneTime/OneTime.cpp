#include <Logme/Logme.h>

struct Test : Logme::OneTimeOverrideGenerator
{
  void Method()
  {
    // Even if this method is called many times, the message below will be printed only once for this object.
    LogmeW(LOGME_ONCE4THIS, "something went wrong!!!");
  }
};

int main()
{
  Test test;

  for (int i = 0; i < 1000; i++)
  {
    test.Method();
  }

  return 0;
}
