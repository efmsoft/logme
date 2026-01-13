#pragma once

#include <Logme/Time/types.h>
#include <Logme/Types.h>

namespace Logme
{
  /**
  * Represents a Date and Time concepts
  */
  class DateTime
  {
  public:

    /**
    * Represents the number of milliseconds per second
    */
    LOGMELNK static TicksType const MillisecondsPerSecond = 1000;

    /**
    * Represents the number of seconds per minute
    */
    LOGMELNK static TicksType const SecondsPerMinute = 60;

    /**
    * Represents the number of minutes per hour
    */
    LOGMELNK static TicksType const MinutesPerHour = 60;

    /**
    * Represents the number of seconds per hour
    */
    LOGMELNK static TicksType const SecondsPerHour = SecondsPerMinute * MinutesPerHour;

    /**
    * Represents the number of hours per day
    */
    LOGMELNK static TicksType const HoursPerDay = 24;

    /**
    * Represents the number of minutes per day
    */
    LOGMELNK static TicksType const MinutesPerDay = MinutesPerHour * HoursPerDay;

    /**
    * Represents the number of secods per day
    */
    LOGMELNK static TicksType const SecondsPerDay = SecondsPerHour * HoursPerDay;

    /**
    * Represents the number of days per week
    */
    LOGMELNK static TicksType const DaysPerWeek = 7;

    /**
    * default ctor
    */
    LOGMELNK DateTime();

    /**
    * Initializes a new instance of the DateTime structure 
    * to the specified year, month, day and kind - UTC or local time.
    */
    LOGMELNK DateTime(DateTimeKind kind, int year, int month, int day);

    /**
    * Initializes a new instance of the DateTime structure 
    * to the specified year, month, day, hour, minute, second, 
    * and Coordinated Universal Time (UTC) or local time. 
    */
    LOGMELNK DateTime(DateTimeKind kind, int year, int month, int day, 
      int hour, int minute, int second, int millisecond = 0);

    /**
    * Gets the milliseconds component of the date 
    * represented by this instance. 
    */
    LOGMELNK int GetMillisecond() const;

    /**
    * Gets the seconds component of the date represented by this instance. 
    */
    LOGMELNK int GetSecond() const;

    /**
    * Gets the minute component of the date represented by this instance. 
    */
    LOGMELNK int GetMinute() const;

    /**
    * Gets the hour component of the date represented by this instance. 
    */
    LOGMELNK int GetHour() const;

    /**
    * Gets the day of the month represented by this instance
    */
    LOGMELNK int GetDay() const;

    /**
    * Gets the day of the week represented by this instance
    */
    LOGMELNK DayOfWeek GetDayOfWeek() const;

    /**
    * Gets the month component of the date represented by this instance. 
    */
    LOGMELNK int GetMonth() const;

    /**
    * Gets the year component of the date represented by this instance. 
    */
    LOGMELNK int GetYear() const;

    /**
    * Gets the day of the year represented by this instance. 
    */
    LOGMELNK int GetDayOfYear() const;

    /**
    * Gets a value that indicates whether the time represented by this 
    * instance is based on local time, Coordinated Universal Time (UTC), 
    * or neither. 
    */
    LOGMELNK DateTimeKind GetKind() const;

    /**
    * Gets a DateTime object that is set to the current date and time 
    * on this computer, expressed as the local time. 
    */
    LOGMELNK static DateTime Now();

    /**
    * Gets a DateTime object that is set to the current date and time 
    * on this computer, expressed as the Coordinated Universal Time (UTC). 
    */
    LOGMELNK static DateTime NowUtc();

    /**
    * Gets the date of this instance, expressed as local time
    * Time of returned value is set as 00::00
    */
    LOGMELNK DateTime GetDateLocal() const;

    /**
    * Gets the date of this instance, expressed as utc time.
    * Time of returned value is set as 00::00
    */
    LOGMELNK DateTime GetDateUtc() const;

    /**
    * Adds the specified number of milliseconds to the value of this instance.  
    */
    LOGMELNK DateTime& AddMilliseconds(TicksType value);

    /**
    * Adds the specified number of seconds to the value of this instance. 
    */
    LOGMELNK DateTime& AddSeconds(TicksType value);

    /**
    * Adds the specified number of minutes to the value of this instance.  
    */
    LOGMELNK DateTime& AddMinutes(TicksType value);

    /**
    * Adds the specified number of hours to the value of this instance.  
    */
    LOGMELNK DateTime& AddHours(TicksType value);

    /**
    * Adds the specified number of days to the value of this instance.
    */
    LOGMELNK DateTime& AddDays(TicksType value);

    /**
    * Adds the specified number of months to the value of this instance.  
    */
    LOGMELNK DateTime& AddMonths(TicksType value);

    /**
    * Adds the specified number of years to the value of this instance.
    */
    LOGMELNK DateTime& AddYears(TicksType value);

    /**
    * Compares two instances of DateTime and returns an indication of their relative values.  
    */
    LOGMELNK int Compare(DateTime const&) const;

    /**
    * Creates a new DateTime object that represents the same time 
    * as the specified DateTime, but is designated in either local time, 
    * Coordinated Universal Time (UTC), or neither, as indicated 
    * by the specified DateTimeKind value.  
    */
    LOGMELNK static DateTime SpecifyKind(DateTime const& value, DateTimeKind kind);

    /**
    * Indicates whether this instance of DateTime is within the 
    * Daylight Saving Time range for the current time zone. 
    */
    LOGMELNK bool IsDaylightSavingTime() const;

    // convert section

    /**
    * Converts the value of the current DateTime object to 
    * Coordinated Universal Time (UTC).
    */
    LOGMELNK DateTime ToUniversalTime() const;

    /**
    * Converts the value of the current DateTime object to local time.  
    */
    LOGMELNK DateTime ToLocalTime() const;

    /**
    * Converts the value of the time_t object to DateTime object 
    * represented in UTC.
    */
    LOGMELNK static DateTime FromTimeT(uni_time_t);

    /**
    * Converts the value to the time_t object 
    */
    LOGMELNK uni_time_t ToTimeT() const;

    /**
    * Converts the value of the std::tm object with local time to DateTime object
    * represented in local time
    */
    LOGMELNK static DateTime FromTmLocal(std::tm const&);

    /**
    * Converts the value of the std::tm object with UTC time to DateTime object
    * represented in UTC
    */
    LOGMELNK static DateTime FromTmUtc(std::tm const&);

    /**
    * Converts the value of the current DateTime object to std::tm represented 
    * as local view.  
    */
    LOGMELNK std::tm ToTmLocal() const;

    /**
    * Converts the value of the current DateTime object to std::tm represented 
    * as UTC view.  
    */
    LOGMELNK std::tm ToTmUtc() const;

    /**
    * Checks whether this instance of DateTime is valid
    */
    bool IsValid() const
    {
      return Valid;
    }

    /**
    * Addition operator
    */
    LOGMELNK DateTime const operator +(TicksType rhs) const;
    
    /**
    * Addition operator
    */
    LOGMELNK DateTime& operator +=(TicksType rhs);

    /**
    * Subtraction operator
    */
    LOGMELNK DateTime const operator -(TicksType rhs) const;

    /**
    * Subtraction operator
    */
    LOGMELNK DateTime& operator -=(TicksType rhs);
    
    /**
    * Subtraction operator. The result of DateTime subtraction 
    * is millisecond interval
    */
    LOGMELNK TicksType operator -(DateTime const& rhs) const;

    /**
    * Comparison < operator
    */
    LOGMELNK bool operator <(DateTime const& rhs) const;
    
    /**
    * Comparison > operator
    */
    LOGMELNK bool operator >(DateTime const& rhs) const;
    
    /**
    * Comparison <= operator
    */
    LOGMELNK bool operator <=(DateTime const& rhs) const;

    /**
    * Comparison >= operator
    */
    LOGMELNK bool operator >=(DateTime const& rhs) const;

    /**
    * Comparison == operator
    */
    LOGMELNK bool operator ==(DateTime const& rhs) const;

    /**
    * Comparison != operator
    */
    LOGMELNK bool operator !=(DateTime const& rhs) const;

    /**
    * Gets minimum supported date for this platform
    */
    LOGMELNK static const DateTime GetMinSupportedDateTime();

    /**
    * Gets maximum supported date for this platform
    */
    LOGMELNK static const DateTime GetMaxSupportedDateTime();

  private:
    /**
    * Ctor for invalid datetime of specified kind
    */
    DateTime(DateTimeKind kind);
    
    /**
    * Adds a milliseconds to this instance
    */
    DateTime& AddValue(TicksType value);

    /**
    * Fills internal datetime representation from values provided
    */
    void FillDateTimes(int year, int month, int day, int hour, int minute,
      int second = 0, int millisecond = 0);

    /**
    * Fills internal datetime representation from milliseconds provided
    */
    void FillTicks(TicksType ticks);

#ifdef _WIN32

    /**
    * ctor for RVO optimization
    */
    DateTime(SystemTime const&, DateTimeKind kind);

    /// Windows time holder
    SystemTime Time;
#else

    /**
    * Fills internal Unix time representation
    */
    void FillComponent();

    /**
    * Fills linear Unix time representation
    */
    void FillLinear();

    /// Unix time holder
    std::tm Time;

    /// Unix time holder in ticks view
    uni_time_t LinearTime;

    TicksType Milliseconds;
#endif

    /// Time representation by this instance - DateTimeKind enum value
    DateTimeKind Kind;

    /// Flag to be true if this instance is logically valid
    bool Valid;
  };

  /**
  * Checks whether two instances of DateTime have same Date not depending
  * on their representation
  */
  LOGMELNK bool AreDatesEqual(DateTime const& lhs, DateTime const& rhs);

  /**
  * Checks whether two instances of DateTime have same Time of day
  * not depending on their representation
  */
  LOGMELNK bool AreTimesEqual(DateTime const& lhs, DateTime const& rhs);

  LOGMELNK unsigned GetTimeInMillisec();
  LOGMELNK void Sleep(unsigned millisec);
}
