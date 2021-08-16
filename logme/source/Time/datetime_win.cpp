#ifdef _WIN32

#include "utils.h"
#include <Logme/time/datetime.h>


#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cassert>

namespace
{
  /// type of pointer to TzSpecificLocalTimeToSystemTime Win API function
  typedef BOOL (WINAPI* FuncType)(LPTIME_ZONE_INFORMATION, LPSYSTEMTIME, PSYSTEMTIME);

  /**
  * Analog of TzSpecificLocalTimeToSystemTime Win API function
  */
  BOOL WINAPI TzSpecificLocalTimeToSystemTimeFake(LPTIME_ZONE_INFORMATION, 
    LPSYSTEMTIME localTime, PSYSTEMTIME uniTime)
  {
    FILETIME ftLocal;
    FILETIME ftUtc;

    return (SystemTimeToFileTime(localTime, &ftLocal) && 
      LocalFileTimeToFileTime(&ftLocal, &ftUtc) &&
      FileTimeToSystemTime(&ftUtc, uniTime));
  }


  /**
  * Returns an address of TzSpecificLocalTimeToSystemTime from kernel32.dll
  * or address of replacement function otherwise.
  */
  FuncType GetTzSpecificLocalTimeToSystemTimePointer()
  {
    static FuncType func = 0;
    if (!func)
    {
      FuncType f = reinterpret_cast<FuncType>(GetProcAddress(GetModuleHandleA("kernel32.dll"), "TzSpecificLocalTimeToSystemTime"));
      func = f ? f : TzSpecificLocalTimeToSystemTimeFake;
    }

    return func;
  }

  /**
  * Tries to add a month to the instance of DateTime class. It keeps a day 
  * of month if possible, i.e. 15 Jan + 1 month == 15 Feb
  * If function cannot keep the day, the round will be performed.
  * 31 Jan + 1 month == 28 Feb (non-leap year)
  * 31 Jan + 1 month == 29 Feb (leap year)
  */
  bool AddMonthMinimal(Logme::DateTime& that)
  {
    // save data
    int day(that.GetDay());
    int month(that.GetMonth());

    // performance speed
    that.AddDays(28);

    int currMonth(that.GetMonth());
    int prevMonth(currMonth);

    while (currMonth == month ||
      (that.GetDay() < day && prevMonth == currMonth))
    {
      prevMonth = currMonth;
      that.AddDays(1);
      currMonth = that.GetMonth();
    }

    // no round will be performed, we've found day needed
    if (that.GetDay() == day)
      return true;

    // round days - back to previous month
    that.AddDays(-1);
    return false;
  }

  /**
  * Tries to subtract a month to the instance of DateTime class. It keeps a day 
  * of month if possible, i.e. 15 Jan - 1 month == 15 Dec
  * If function cannot keep the day, the round will be performed.
  * 31 Mar - 1 month == 28 Feb (non-leap year)
  */
  bool SubMonthMinimal(Logme::DateTime& that)
  {
    int day(that.GetDay());

    // back to previous month
    that.AddDays(-that.GetDay());

    // round day ?
    if (that.GetDay() < day)
      return false;

    // find appropriate day
    do 
    {
      that.AddDays(-1);
    } 
    while (that.GetDay() != day && that.IsValid());

    return true;
  }
}

namespace Logme
{
  void DateTime::FillDateTimes(int year, int month, int day, int hour, 
    int minute, int second /* = 0 */, int millisecond /* = 0 */)
  {
    Time.Milliseconds = static_cast<uint16_t>(millisecond);
    Time.Second = static_cast<uint16_t>(second);
    Time.Minute = static_cast<uint16_t>(minute);
    Time.Hour = static_cast<uint16_t>(hour);
    Time.Day = static_cast<uint16_t>(day);
    Time.Month = static_cast<uint16_t>(month);
    Time.Year = static_cast<uint16_t>(year);

    AddValue(0);
  }

  DateTime::DateTime(SystemTime const& st, DateTimeKind kind) 
    : Time(st), Kind(kind), Valid(true)
  {
  }

  //////////////////////////////////////////////////////////////////////////

  DateTime& DateTime::AddValue(long long value)
  {
    assert(IsValid());
    if (!IsValid())
    {
      return *this;
    }

    TicksType const ticksPerMillisecond = 10000;

    FILETIME ft;
    if (SystemTimeToFileTime(reinterpret_cast<SYSTEMTIME*>(&Time), &ft))
    {
      // update time value
      reinterpret_cast<ULARGE_INTEGER&>(ft).QuadPart += value * ticksPerMillisecond;

      if (!FileTimeToSystemTime(&ft, reinterpret_cast<SYSTEMTIME*>(&Time)))
      {
        Valid = false;
      }
    }
    else
      Valid = false;

    return *this;
  }

  DateTime& DateTime::AddSeconds(TicksType value)
  {
    TicksType mulValue(value * MillisecondsPerSecond);
    bool noOverflow((mulValue < 0 && value < 0) || 
      (mulValue >= 0 && value >= 0));

    if (noOverflow)
    {
      return AddMilliseconds(mulValue);
    }
    else
    {
      Valid = false;
      return *this;
    }
  }

  DateTime& DateTime::AddMinutes(TicksType value)
  {
    TicksType mulValue(value * SecondsPerMinute);
    bool noOverflow((mulValue < 0 && value < 0) || 
      (mulValue >= 0 && value >= 0));

    if (noOverflow)
    {
      return AddSeconds(mulValue);
    }
    else
    {
      Valid = false;
      return *this;
    }
  }

  DateTime& DateTime::AddHours(TicksType value)
  {
    TicksType mulValue(value * MinutesPerHour);
    bool noOverflow((mulValue < 0 && value < 0) || 
      (mulValue >= 0 && value >= 0));

    if (noOverflow)
    {
      int dstOld(IsDaylightSavingTime());
      AddMinutes(mulValue);
      int dstNew(IsDaylightSavingTime());

      // if DST changed, correct hour
      if (Kind != DTK_UTC && dstOld ^ dstNew)
      {
        AddMinutes(MinutesPerHour * (dstOld ? -1 : 1));
      }
    }
    else
    {
      Valid = false;
    }

    return *this;
  }

  DateTime& DateTime::AddDays(TicksType value)
  {
    TicksType mulValue(value * HoursPerDay * MinutesPerHour);
    bool noOverflow((mulValue < 0 && value < 0) || 
      (mulValue >= 0 && value >= 0));

    if (noOverflow)
    {
      return AddMinutes(mulValue);
    }
    else
    {
      Valid = false;
      return *this;
    }
  }

  DateTime& DateTime::AddMonths(TicksType value)
  {
    if (value >= 0)
    {
      for (TicksType i(0); i != value; ++i)
        AddMonthMinimal(*this);
    }
    else
    {
      for (TicksType i(0); i != value; --i)
        SubMonthMinimal(*this);
    }

    return *this;
  }

  DateTime& DateTime::AddYears(TicksType value)
  {
    // save data
    int day(GetDay());
    int month(GetMonth());

    // some calendars have 13 months, so we have to increase time to 12 
    // months and decide later what to do
    AddMonths(12 * value);

    if (GetMonth() == month)
    {
      // we have one of the common calendars, return
      return *this;
    }
    else
    {
      // for calendars with more than 12 months
      while (GetDay() != day || GetMonth() != month)
        AddDays(1);
    }

    return *this;
  }

  int DateTime::GetMillisecond() const
  {
    assert(IsValid());
    return Time.Milliseconds;
  }

  int DateTime::GetSecond() const
  {
    assert(IsValid());
    return Time.Second;
  }

  int DateTime::GetMinute() const
  {
    assert(IsValid());
    return Time.Minute;
  }

  int DateTime::GetHour() const
  {
    assert(IsValid());
    return Time.Hour;
  }

  int DateTime::GetDay() const
  {
    assert(IsValid());
    return Time.Day;
  }

  DayOfWeek DateTime::GetDayOfWeek() const
  {
    assert(IsValid());
    return static_cast<DayOfWeek>(Time.DayOfWeek);
  }

  int DateTime::GetMonth() const
  {
    assert(IsValid());
    return Time.Month;
  }

  int DateTime::GetYear() const
  {
    assert(IsValid());
    return Time.Year;
  }

  //////////////////////////////////////////////////////////////////////////

  DateTime DateTime::FromTimeT(uni_time_t t)
  {
    SystemTime newTime;

    return UnixTimeToSystemTime(t, newTime) ? 
      DateTime(newTime, DTK_UTC) : DateTime(DTK_UTC);
  }

  uni_time_t DateTime::ToTimeT() const
  {
    assert(IsValid());
    if (!IsValid())
      return 0;
	  
	  if (Kind != DTK_UTC)
    {
    {
      DateTime utc = ToUniversalTime();
      return (utc.IsValid() && (utc.GetKind() == DTK_UTC)) ? utc.ToTimeT() : 0;
    }
    }

    uni_time_t temp(0);
    return SystemTimeToUnixTime(Time, temp) ? temp : 0;
  }

  DateTime DateTime::ToLocalTime() const
  {
    assert(IsValid());
    if (!IsValid())
    {
      return DateTime(DTK_LOCAL);
    }

    if (Kind == DTK_LOCAL)
      return *this;

    SystemTime newTime;

    return SystemTimeToTzSpecificLocalTime(0, 
      reinterpret_cast<SYSTEMTIME*>(const_cast<SystemTime*>(&Time)), 
      reinterpret_cast<SYSTEMTIME*>(&newTime)) ? DateTime(newTime, DTK_LOCAL) : DateTime(DTK_LOCAL);
  }

  DateTime DateTime::ToUniversalTime() const
  {
    assert(IsValid());
    if (!IsValid())
    {
      return DateTime(DTK_UTC);
    }

    if (Kind == DTK_UTC)
      return *this;

    /// Due to absence TzSpecificLocalTimeToSystemTime in Windows 2000,
    /// we should check kernel32.dll before. If doesn't contain
    /// this function, we have to call our analog function

    FuncType func = GetTzSpecificLocalTimeToSystemTimePointer();

    /// used to store utc time
    SystemTime newTime;

    SYSTEMTIME* local(reinterpret_cast<SYSTEMTIME*>(const_cast<SystemTime*>(&Time))); 
    SYSTEMTIME* utc(reinterpret_cast<SYSTEMTIME*>(&newTime));

    return func(0, local, utc) ? DateTime(newTime, DTK_UTC) : DateTime(DTK_UTC);
  }

  std::tm DateTime::ToTmLocal() const
  {
    assert(IsValid());
    uni_time_t temp(ToUniversalTime().ToTimeT());
    
    std::tm tempTime;
    _localtime64_s(&tempTime, &temp);

    return _localtime64_s(&tempTime, &temp) == 0 ? tempTime : std::tm();
  }

  std::tm DateTime::ToTmUtc() const
  {
    assert(IsValid());
    
    uni_time_t temp(ToUniversalTime().ToTimeT());

    std::tm tempTime;
    return _gmtime64_s(&tempTime, &temp) == 0 ? tempTime : std::tm();
  }

  DateTime DateTime::Now()
  {
    SYSTEMTIME temp;
    GetSystemTime(&temp);

    SystemTime local;

    return SystemTimeToTzSpecificLocalTime(0, &temp, reinterpret_cast<SYSTEMTIME*>(&local)) 
      ? DateTime(local, DTK_LOCAL) : DateTime(DTK_LOCAL);
  }

  DateTime DateTime::NowUtc()
  {
    SystemTime temp;
    GetSystemTime(reinterpret_cast<SYSTEMTIME*>(&temp));

    return DateTime(temp, DTK_UTC);
  }

  TicksType DateTime::operator -(DateTime const& rhs) const
  {
    TicksType const ticksPerMillisecond = 10000;
    if (!IsValid() || !rhs.IsValid())
      return 0;

    if (Kind != rhs.GetKind())
      return operator -(Kind == DTK_UTC ? rhs.ToUniversalTime() : rhs.ToLocalTime());

    FILETIME thisTime;
    FILETIME thatTime;

    if ((SystemTimeToFileTime(reinterpret_cast<SYSTEMTIME*>(const_cast<SystemTime*>(&Time)), &thisTime) &&
      SystemTimeToFileTime(reinterpret_cast<SYSTEMTIME*>(const_cast<SystemTime*>(&rhs.Time)), &thatTime)))
    {
      TicksType const left = reinterpret_cast<ULARGE_INTEGER*>(&thisTime)->QuadPart;
      TicksType const right = reinterpret_cast<ULARGE_INTEGER*>(&thatTime)->QuadPart;
      return (left - right) / ticksPerMillisecond;
    }
    else
    {
      return 0;
    }
  }

  /// comparison operators

  int DateTime::Compare(DateTime const& rhs) const
  {
    assert(IsValid() && rhs.IsValid());
    if (!IsValid() || !rhs.IsValid())
    {
      return 1;
    }

    if (Kind != rhs.GetKind())
      return Compare(Kind == DTK_UTC ? rhs.ToUniversalTime() : rhs.ToLocalTime());

    FILETIME myTime;
    SystemTimeToFileTime(reinterpret_cast<SYSTEMTIME*>(const_cast<SystemTime*>(&Time)), &myTime);

    FILETIME rhsTime;
    SystemTimeToFileTime(reinterpret_cast<SYSTEMTIME*>(const_cast<SystemTime*>(&rhs.Time)), &rhsTime);

    return CompareFileTime(&myTime, &rhsTime);
  }

  const DateTime DateTime::GetMinSupportedDateTime()
  {
    return DateTime(DTK_UTC, 1900, 1, 1, 0, 0, 0, 0);
  }

  const DateTime DateTime::GetMaxSupportedDateTime()
  {
    return DateTime(DTK_UTC, 9999, 1, 1, 0, 0, 0, 0);
  }

  void Logme::Sleep(unsigned millisec)
  {
    ::Sleep(millisec);
  } 
}

#endif // #ifdef _WIN32
