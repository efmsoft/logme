#include <Logme/Time/datetime.h>
#include <cassert>

namespace Logme
{
  DateTime::DateTime() 
    : Kind(DTK_LOCAL), Valid(false)
  {
  }

  DateTime::DateTime(DateTimeKind kind) 
    : Kind(kind), Valid(false)
  {
  }

  DateTime::DateTime(DateTimeKind kind, int year, int month, int day) 
    : Kind(kind), Valid(true)
  {
    FillDateTimes(year, month, day, 0, 0, 0, 0);
  }

  DateTime::DateTime(DateTimeKind kind, int year, int month, int day, 
    int hour, int minute, int second, int millisecond)
    : Kind(kind), Valid(true)
  {
    FillDateTimes(year, month, day, hour, minute, second, millisecond);
  }

  DateTimeKind DateTime::GetKind() const
  {
    return Kind;
  }

  int DateTime::GetDayOfYear() const
  {
    assert(IsValid());

    TicksType const oneDayLength = DateTime::MillisecondsPerSecond * DateTime::SecondsPerDay;

    DateTime begin(DTK_UTC, GetYear(), 1, 1);
    DateTime end(DTK_UTC, GetYear(), GetMonth(), GetDay());

    TicksType const delta = end - begin;

    // One day may have only 23 hours, so we must perform a little correction
    return static_cast<int>(1 + (delta + oneDayLength / 2) / oneDayLength);
  }

  DateTime DateTime::GetDateLocal() const
  {
    assert(IsValid());
    DateTime temp(ToLocalTime());

    return DateTime(DTK_LOCAL, temp.GetYear(), temp.GetMonth(), temp.GetDay(), 0, 0, 0, 0);
  }

  DateTime DateTime::GetDateUtc() const
  {
    assert(IsValid());
    DateTime temp(ToUniversalTime());

    return DateTime(DTK_UTC, temp.GetYear(), temp.GetMonth(), temp.GetDay(), 0, 0, 0, 0);
  }

  DateTime DateTime::SpecifyKind(DateTime const& value, DateTimeKind kind)
  {
    assert(value.IsValid());
    if (!value.IsValid())
    {
      return DateTime(kind);
    }

    DateTimeKind kindOld = value.GetKind();
    if (kindOld == kind)
    {
      return value;
    }

    return DateTime(kind, value.GetYear(), value.GetMonth(), value.GetDay(), value.GetHour(), value.GetMinute(), value.GetSecond(), value.GetMillisecond());
  }

  bool DateTime::IsDaylightSavingTime() const
  {
    assert(IsValid());
    return IsValid() ? ToTmLocal().tm_isdst == 1 : false;
  }

  //////////////////////////////////////////////////////////////////////////

  DateTime& DateTime::AddMilliseconds(TicksType value)
  {
    return AddValue(value);
  }

  DateTime& DateTime::operator +=(TicksType rhs)
  {
    return AddValue(rhs);
  }

  DateTime& DateTime::operator -=(TicksType rhs)
  {
    return AddValue(-rhs);
  }

  DateTime DateTime::FromTmLocal(std::tm const& tmLocal)
  {
    return DateTime(DTK_LOCAL, tmLocal.tm_year + 1900, tmLocal.tm_mon + 1,
      tmLocal.tm_mday, tmLocal.tm_hour, tmLocal.tm_min, tmLocal.tm_sec, 0);
  }

  DateTime DateTime::FromTmUtc(std::tm const& tmUtc)
  {
    return DateTime(DTK_UTC, tmUtc.tm_year + 1900, tmUtc.tm_mon + 1,
      tmUtc.tm_mday, tmUtc.tm_hour, tmUtc.tm_min, tmUtc.tm_sec, 0);
  }

  //////////////////////////////////////////////////////////////////////////

  DateTime const DateTime::operator +(TicksType rhs) const
  {
    return DateTime(*this) += rhs;
  }

  DateTime const DateTime::operator -(TicksType rhs) const
  {
    return DateTime(*this) -= rhs;
  }

  bool DateTime::operator ==(DateTime const& rhs) const
  {
    return !Compare(rhs);
  }

  bool DateTime::operator !=(DateTime const& rhs) const
  {
    return !operator==(rhs);
  }

  bool DateTime::operator <(DateTime const& rhs) const
  {
    return Compare(rhs) < 0;
  }

  bool DateTime::operator >=(DateTime const& rhs) const
  {
    return !operator <(rhs);
  }

  bool DateTime::operator >(DateTime const& rhs) const
  {
    return Compare(rhs) > 0;
  }

  bool DateTime::operator <=(DateTime const& rhs) const
  {
    return !operator >(rhs);
  }

  bool AreDatesEqual(DateTime const& lhs, DateTime const& rhs)
  {
    assert(lhs.IsValid() && rhs.IsValid());
    if (!lhs.IsValid() || !rhs.IsValid())
    {
      return false;
    }

    return lhs.GetDateUtc() == rhs.GetDateUtc();
  }

  bool AreTimesEqual(DateTime const& lhs, DateTime const& rhs)
  {
    assert(lhs.IsValid() && rhs.IsValid());
    if (!lhs.IsValid() || !rhs.IsValid())
    {
      return false;
    }

    DateTime lhsUtc(lhs.ToUniversalTime());
    DateTime rhsUtc(rhs.ToUniversalTime());

    return lhs.GetHour() == rhs.GetHour() &&
      lhs.GetMinute() == rhs.GetMinute()  &&
      lhs.GetSecond() == rhs.GetSecond()  &&
      lhs.GetMillisecond() == rhs.GetMillisecond();
  }
}
