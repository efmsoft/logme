#pragma once

#include <ctime>

namespace Logme
{
  class DayChangeDetector
  {
    time_t StartOfDayTime;
    time_t NextMidnightTime;
  
  public:
    DayChangeDetector();

    bool IsSameDayCached();
    void UpdateDayBoundaries();
  };
}