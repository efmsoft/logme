#pragma once

#include <mutex>
#include <stdint.h>
#include <string>

#include <Logme/Types.h>

namespace Logme
{
  class CS
  {
#if LOGME_CS_USE_CRITICAL_SECTION
    union
    {
      uint8_t SectionData[256];
  #ifdef _WINDOWS_
      CRITICAL_SECTION CriticalSection;
  #endif
    };
#else
    std::recursive_mutex Mutex;
#endif

  public:
    LOGMELNK CS();
    LOGMELNK ~CS();

    class AutoLock
    {
      friend CS;
      CS* Section;

    public:
      LOGMELNK AutoLock(AutoLock&& src) noexcept;
      LOGMELNK ~AutoLock();

      LOGMELNK void Release();
      LOGMELNK operator bool() const;

    private:
      AutoLock() = delete;
      AutoLock(const AutoLock&) = delete;
      AutoLock(CS* section, bool tryLock);

      AutoLock& operator=(const AutoLock&) = delete;
    };

    LOGMELNK const AutoLock Lock();
    LOGMELNK const AutoLock TryLock();

    LOGMELNK bool TryAcquire();
    LOGMELNK void Acquire();
    LOGMELNK void Release();

    LOGMELNK void lock();
    LOGMELNK void unlock();

  private:
    CS(const CS&) = delete;
    CS(CS&& src) noexcept = delete;
    CS& operator=(const CS&) = delete;
  };
}
