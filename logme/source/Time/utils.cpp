#include "utils.h"

#include <Logme/Time/datetime.h>

#if defined(__GNUC__) && !defined(__DJGPP__)
#include <sys/time.h>
#endif

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

using Logme::uni_time_t;
using Logme::DateTime;

//FILETIME unit of measure is 100 nanoseconds, and unix time unit of measure is second. 1 second = 10000000 FILETIME time quantums
const ULONGLONG TimeQuantumsInSecond = 10000000UL;
// 134774 count of days from 1-1-1601 (initial FILETIME value) until the 1-1-1970 (initial unix time value)
const ULONGLONG UnixTimeStartValue = TimeQuantumsInSecond * static_cast<ULONGLONG>(DateTime::SecondsPerMinute)
                                                          * static_cast<ULONGLONG>(DateTime::MinutesPerHour)
                                                          * static_cast<ULONGLONG>(DateTime::HoursPerDay) * 134774ULL;

void UnixTimeToFileTime(uni_time_t time, LPFILETIME fileTime)
{
  LONGLONG longTime = static_cast<LONGLONG>(time * TimeQuantumsInSecond + UnixTimeStartValue);
  fileTime->dwLowDateTime = static_cast<DWORD>(longTime);
  fileTime->dwHighDateTime = static_cast<DWORD>(longTime >> (sizeof(DWORD) * 8));
}

bool FileTimeToUnixTime(const FILETIME& fileTime, uni_time_t& time)
{
  time = static_cast<uni_time_t>(-1);
  bool ret = false;
  LONGLONG longTime = (static_cast<LONGLONG>(fileTime.dwHighDateTime) << (sizeof(DWORD) * 8)) + fileTime.dwLowDateTime;
  longTime = (longTime - UnixTimeStartValue) / TimeQuantumsInSecond;
  if (longTime > 0)
  {
    time = static_cast<uni_time_t>(longTime);
    ret = true;
  }
  return ret;
}

bool Logme::UnixTimeToSystemTime(const uni_time_t& time, SystemTime& stime)
{
  FILETIME ft;
  BOOLEAN success = FALSE;
  HINSTANCE hinstLib = GetModuleHandle("Ntdll.dll");
  if (hinstLib)
  {
    typedef VOID (WINAPI *RtlSecondsSince1970ToTime)(ULONG, PLARGE_INTEGER);
    RtlSecondsSince1970ToTime convertFunc = reinterpret_cast<RtlSecondsSince1970ToTime>(GetProcAddress(hinstLib, "RtlSecondsSince1970ToTime")); 
    if (convertFunc)
    {
      convertFunc(static_cast<ULONG>(time), reinterpret_cast<PLARGE_INTEGER>(&ft));
      success = TRUE;
    }
  }
  if (!success)
  {
    UnixTimeToFileTime(time, &ft);
  }
  return !!FileTimeToSystemTime(&ft, reinterpret_cast<SYSTEMTIME*>(&stime));
}

#ifdef _WIN32
bool Logme::SystemTimeToUnixTime(const SystemTime& stime, uni_time_t& time)
{
  time = static_cast<uni_time_t>(-1);
  FILETIME ft = {0};
  BOOLEAN success = SystemTimeToFileTime(reinterpret_cast<const SYSTEMTIME*>(&stime), &ft);
  if (!success)
  {
    return false;
  }
  HINSTANCE hinstLib = GetModuleHandle("Ntdll.dll");
  if (hinstLib)
  {
    typedef BOOLEAN (WINAPI *RtlTimeToSecondsSince1970)(PLARGE_INTEGER, PULONG);

    RtlTimeToSecondsSince1970 convertFunc = reinterpret_cast<RtlTimeToSecondsSince1970>(GetProcAddress(hinstLib, "RtlTimeToSecondsSince1970")); 
    if (convertFunc)
    {
      ULONG uLong(0);
      success = convertFunc(reinterpret_cast<PLARGE_INTEGER>(&ft), &uLong);
      time = static_cast<uni_time_t>(uLong);
    }
  }
  if (!success)
  {
    return FileTimeToUnixTime(ft, time);
  }
  return true;
}

bool Logme::DateTimeToSystemTime(const DateTime& time, SystemTime& stime)
{
  if (!time.IsValid())
  {
    return false;
  }
  stime.Year = time.GetYear();
  stime.Month = time.GetMonth();
  stime.DayOfWeek = time.GetDayOfWeek();
  stime.Day = time.GetDay();
  stime.Hour = time.GetHour();
  stime.Minute = time.GetMinute();
  stime.Second = time.GetSecond();
  stime.Milliseconds = time.GetMillisecond();
  return true;
}
#endif

#endif // #ifdef _WIN32

unsigned Logme::GetTimeInMillisec()
{
#if defined(__GNUC__) && !defined(__DJGPP__)
  timeval now;
  gettimeofday(&now, 0);
  return now.tv_sec * 1000 + now.tv_usec / 1000;
#elif defined(_WIN32) || defined(_WIN64)
  return ::GetTickCount();
#elif (CLOCKS_PER_SEC == 1000)
  return clock();
#else
  unsigned clocks = clock();
  unsigned tmp = clocks * 1000;
  if (tmp > clocks)
  {
    return tmp / CLOCKS_PER_SEC;
  }
  else
  {
    return clocks * (1000 / CLOCKS_PER_SEC);
  }
#endif
} 
