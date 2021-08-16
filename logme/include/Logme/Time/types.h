#pragma once

#include <ctime>
#include <stdint.h>

namespace Logme
{
  typedef long long TicksType;

#ifdef _MSC_VER
  typedef __time64_t uni_time_t;
#else
  typedef time_t uni_time_t;
#endif

  enum DateTimeKind
  {
    DTK_LOCAL       = 0x01, ///< The time represented is local time. 
    DTK_UTC         = 0x02, ///< The time represented is UTC. 
  };

  enum DayOfWeek
  {
    DAY_SUNDAY    = 0x00,
    DAY_MONDAY    = 0x01,
    DAY_TUESDAY   = 0x02,
    DAY_WEDNESDAY = 0x03,
    DAY_THURSDAY  = 0x04,
    DAY_FRIDAY    = 0x05,
    DAY_SATURDAY  = 0x06,
  };

#ifdef _WIN32
  struct SystemTime 
  {
    uint16_t Year;
    uint16_t Month;
    uint16_t DayOfWeek;
    uint16_t Day;
    uint16_t Hour;
    uint16_t Minute;
    uint16_t Second;
    uint16_t Milliseconds;

    SystemTime() : Year(0), Month(0), DayOfWeek(0), 
      Day(0), Hour(0), Minute(0), Second(0), Milliseconds(0) {}
  };
#endif
}
