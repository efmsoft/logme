#ifndef _WIN32

#include <Logme/Time/datetime.h>
#include <Logme/Time/reentrant.h>

#include <sys/time.h>
#include <cassert>

namespace
{
  int GetUtcAndDstOffset(Logme::uni_time_t myTime)
  {
    std::tm utcTime;
    gmtime_r(&myTime, &utcTime);
    std::tm localTime;
    localtime_r(&myTime, &localTime);
    // The difference between local time and UTC time depends on time of the year
    // If we will not set the tm_isdst to zero, the obtained offset will not depend on time of the year. 
    // Example: For Moscow the offset will be -4/-3 depending on summer time/winter time
    localTime.tm_isdst = 0;
    return mktime(&utcTime) - mktime(&localTime);
  }
}

namespace Logme
{
  void DateTime::FillDateTimes(int year, int month, int day, int hour, int minute, int second /* = 0 */, int millisecond/* = 0 */)
  {
    Time.tm_sec  = second;
    Time.tm_min  = minute;
    Time.tm_hour = hour;
    Time.tm_mday = day;
    Time.tm_mon = month - 1;
    Time.tm_year = year - 1900;
    Time.tm_wday = -1;
    Time.tm_yday = -1;
    Milliseconds = millisecond;

    FillLinear();
  }

  //////////////////////////////////////////////////////////////////////////

  DateTime& DateTime::AddValue(long long value)
  {
    assert(IsValid());
    if (!IsValid())
    {
      return *this;
    }

    TicksType temp(LinearTime * MillisecondsPerSecond + value + Milliseconds);
    if (temp < 0)
    {
      // overflow
      Valid = false;
      return *this;
    }
    else
    {
      LinearTime = temp / MillisecondsPerSecond;
      Milliseconds = temp % MillisecondsPerSecond;
    }

    FillComponent();

    return *this;
  }

  DateTime& DateTime::AddSeconds(TicksType value)
  {
    FillDateTimes(GetYear(), GetMonth(), GetDay(), GetHour(), GetMinute(),
      GetSecond() + value, GetMillisecond());
    
    return *this;
  }

  DateTime& DateTime::AddMinutes(TicksType value)
  {
    FillDateTimes(GetYear(), GetMonth(), GetDay(), GetHour(), GetMinute() + value,
      GetSecond(), GetMillisecond());

    return *this;
  }

  DateTime& DateTime::AddHours(TicksType value)
  {
    if (value != static_cast<int>(value))
    {
      // overflow
      Valid = false;
      return *this;
    }

    int dstOld(IsDaylightSavingTime());

    FillDateTimes(GetYear(), GetMonth(), GetDay(), GetHour() + value, GetMinute(),
      GetSecond(), GetMillisecond());

    int dstNew(IsDaylightSavingTime());

    // if DST changed, correct hour
    if (Kind != DTK_UTC && dstOld ^ dstNew)
    {
      AddMinutes(MinutesPerHour * (dstOld ? -1 : 1));
    }

    return *this;
  }

  DateTime& DateTime::AddDays(TicksType value)
  {
    if (value != static_cast<int>(value))
    {
      //overflow
      Valid = false;
      return *this;
    }

    FillDateTimes(GetYear(), GetMonth(), GetDay() + value, GetHour(), GetMinute(),
      GetSecond(), GetMillisecond());

    return *this;
  }

  DateTime& DateTime::AddMonths(TicksType value)
  {
    if (value != static_cast<int>(value))
    {
      // overflow
      Valid = false;
      return *this;
    }

    FillDateTimes(GetYear(), GetMonth() + value, GetDay(), GetHour(), GetMinute(),
      GetSecond(), GetMillisecond());

    return *this;
  }

  DateTime& DateTime::AddYears(TicksType value)
  {
    if (value != static_cast<int>(value))
    {
      // overflow
      Valid = false;
      return *this;
    }

    FillDateTimes(GetYear() + value, GetMonth(), GetDay(), GetHour(), GetMinute(),
      GetSecond(), GetMillisecond());

    return *this;
  }

  void DateTime::FillComponent()
  {
    tm t;
    tm* res = (Kind == DTK_UTC) ? gmtime_r(&LinearTime, &t) : localtime_r(&LinearTime, &t);
    if (res)
    {
      Time = t;
      Valid = true;
    }
    else
      Valid = false;
  }

  void DateTime::FillLinear()
  {
    Time.tm_isdst = -1;
    LinearTime = mktime(&Time);
    if (Kind == DTK_UTC)
    {
      int offset = GetUtcAndDstOffset(LinearTime);
      Time.tm_sec -= offset;
      LinearTime -= offset;
    }

    Valid = LinearTime != static_cast<uni_time_t>(-1);

    FillComponent();
  }

  int DateTime::GetMillisecond() const
  {
    assert(IsValid());
    return Milliseconds;
  }

  int DateTime::GetSecond() const
  {
    assert(IsValid());
    return Time.tm_sec;
  }

  int DateTime::GetMinute() const
  {
    assert(IsValid());
    return Time.tm_min;
  }

  int DateTime::GetHour() const
  {
    assert(IsValid());
    return Time.tm_hour;
  }

  int DateTime::GetDay() const
  {
    assert(IsValid());
    return Time.tm_mday;
  }

  DayOfWeek DateTime::GetDayOfWeek() const
  {
    assert(IsValid());
    return static_cast<DayOfWeek>(Time.tm_wday);
  }

  int DateTime::GetMonth() const
  {
    assert(IsValid());
    return Time.tm_mon + 1;
  }

  int DateTime::GetYear() const
  {
    assert(IsValid());
    return Time.tm_year + 1900;
  }

  DateTime DateTime::FromTimeT(uni_time_t t)
  {
    std::tm myTime;

    if (!gmtime_r(&t, &myTime))
    {
      return DateTime(DTK_UTC);
    }

    return DateTime(DTK_UTC, myTime.tm_year + 1900, myTime.tm_mon + 1,
      myTime.tm_mday, myTime.tm_hour, myTime.tm_min, myTime.tm_sec, 0);
  }

  uni_time_t DateTime::ToTimeT() const
  {
    assert(IsValid());
    if (!IsValid())
      return 0;

    return LinearTime;
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

    struct tm myTime;

    if (!localtime_r(&LinearTime, &myTime))
    {
      return DateTime(DTK_LOCAL);
    }

    return DateTime(DTK_LOCAL, myTime.tm_year + 1900, myTime.tm_mon + 1,
      myTime.tm_mday, myTime.tm_hour, myTime.tm_min, myTime.tm_sec, Milliseconds);
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

    std::tm myTime;
    if (!gmtime_r(&LinearTime, &myTime))
    {
      return DateTime(DTK_UTC);
    }

    return DateTime(DTK_UTC, myTime.tm_year + 1900, myTime.tm_mon + 1,
      myTime.tm_mday, myTime.tm_hour, myTime.tm_min, myTime.tm_sec, Milliseconds);
  }

  std::tm DateTime::ToTmLocal() const
  {
    return Kind == DTK_LOCAL ? Time : ToLocalTime().ToTmLocal();
  }

  std::tm DateTime::ToTmUtc() const
  {
    return Kind == DTK_UTC ? Time : ToUniversalTime().ToTmUtc();
  }

  TicksType DateTime::operator -(DateTime const& rhs) const
  {
    return static_cast<TicksType>(difftime(LinearTime, rhs.LinearTime) * MillisecondsPerSecond)
      + Milliseconds - rhs.Milliseconds;
  }

  DateTime DateTime::Now()
  {
    TicksType milliseconds(0);

    time_t temp;

    timeval now;
    if (!gettimeofday(&now, 0))
    {
      temp = now.tv_sec;
      milliseconds = now.tv_usec / 1000;
    }
    else
    {
      time(&temp);
    }

    std::tm myTime;
    if (!localtime_r(&temp, &myTime))
    {
      return DateTime(DTK_LOCAL);
    }

    return DateTime(DTK_LOCAL, myTime.tm_year + 1900, myTime.tm_mon + 1,
      myTime.tm_mday, myTime.tm_hour, myTime.tm_min, myTime.tm_sec, milliseconds);
  }

  DateTime DateTime::NowUtc()
  {
    TicksType milliseconds(0);

    time_t temp;

    timeval now;
    if (!gettimeofday(&now, 0))
    {
      temp = now.tv_sec;
      milliseconds = now.tv_usec / 1000;
    }
    else
    {
      time(&temp);
    }

    std::tm myTime;
    if (!gmtime_r(&temp, &myTime))
    {
      return DateTime(DTK_UTC);
    }

    return DateTime(DTK_UTC, myTime.tm_year + 1900, myTime.tm_mon + 1,
      myTime.tm_mday, myTime.tm_hour, myTime.tm_min, myTime.tm_sec, milliseconds);
  }

  /// comparison operators

  int DateTime::Compare(DateTime const& rhs) const
  {
    if (!IsValid() || !rhs.IsValid())
    {
      return 1;
    }

    if (Kind != rhs.GetKind())
      return Compare(Kind == DTK_UTC ? rhs.ToUniversalTime() : rhs.ToLocalTime());

    TicksType result = (LinearTime - rhs.LinearTime) * MillisecondsPerSecond 
      + Milliseconds - rhs.Milliseconds;

    return !result ? 0 : result > 0 ? 1 : -1;
  }

  const DateTime DateTime::GetMinSupportedDateTime()
  {
    return DateTime(DTK_UTC, 1970, 1, 1, 0, 0, 0, 0);
  }

  const DateTime DateTime::GetMaxSupportedDateTime()
  {
    return DateTime(DTK_UTC, 2038, 1, 18, 0, 0, 0, 0);
  }

  void Sleep(unsigned millisec)
  {
    timespec delay;
    timespec remain;
    unsigned usec = (millisec % 1000) * 1000;
    delay.tv_sec = millisec / 1000 + usec / 1000000;
    delay.tv_nsec = (usec % 1000000) * 1000;
    while (nanosleep(&delay, &remain) != 0)
    {
      delay = remain;
    }
  } 
}

#endif
