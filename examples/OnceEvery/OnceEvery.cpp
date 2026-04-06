#include <Logme/Logme.h>
#include <chrono>
#include <thread>

int main()
{
  for (int i = 0; i < 5; i++)
  {
    LogmeI_Once("LogmeI_Once: i=%d", i);
  }

  for (int i = 0; i < 5; i++)
  {
    LogmeI_Once(::CH, ::SUBSID, "LogmeI_Once with CH/SUBSID: i=%d", i);
  }

  for (int i = 0; i < 10; i++)
  {
    LogmeI_Every(300, "LogmeI_Every: i=%d", i);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  for (int i = 0; i < 5; i++)
  {
    fLogmeI_Once("fLogmeI_Once: i={}", i);
  }

  for (int i = 0; i < 5; i++)
  {
    fLogmeI_Once(::CH, "fLogmeI_Once with CH: i={}", i);
  }

  for (int i = 0; i < 10; i++)
  {
    fLogmeI_Every(300, "fLogmeI_Every: i={}", i);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return 0;
}
