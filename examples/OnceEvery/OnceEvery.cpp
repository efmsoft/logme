#include <Logme/Logme.h>
#include <chrono>
#include <thread>

static void OnceGlobal(bool first)
{
  for (int i = 0; i < 5; i++)
    LogmeI_Once("LogmeI_Once() [%s call]: i=%d", first ? "first" : "second", i);
}

static void OnceScope(bool first)
{
  LOGME_CALL_SCOPE;

  for (int i = 0; i < 5; i++)
    LogmeI(LOGME_ONCE4CALL, "LogmeI(LOGME_ONCE4CALL) [%s call]: i=%d", first ? "first" : "second", i);
}

static void EveryGlobal(bool first, int i, int j)
{
  for (; i < j; i++)
  {
    LogmeI_Every(300, "LogmeI_Every(300ms) [%s]: i=%d", first ? "first" : "second", i);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

int main()
{
  OnceGlobal(true);
  OnceGlobal(false);

  OnceScope(true);
  OnceScope(false);

  EveryGlobal(true, 0, 10);
  EveryGlobal(false, 10, 20);

  return 0;
}
