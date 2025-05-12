#include <Logme/DayChangeDetector.h>

using namespace Logme;

DayChangeDetector::DayChangeDetector()
  : StartOfDayTime(0)
  , NextMidnightTime(0)
{
  UpdateDayBoundaries();
}

bool DayChangeDetector::IsSameDayCached()
{
  const time_t now = time(nullptr);
  
  if (now < NextMidnightTime)
    return true;

  UpdateDayBoundaries();
  return false;
}

void DayChangeDetector::UpdateDayBoundaries()
{
  const time_t now = time(nullptr);
  
  struct tm tmNow {};
#ifdef _WIN32
  localtime_s(&tmNow, &now);
#else
  localtime_r(&now, &tmNow);
#endif
  
  // Set to midnight of today
  tmNow.tm_hour = 0;
  tmNow.tm_min = 0;
  tmNow.tm_sec = 0;
  StartOfDayTime = mktime(&tmNow);

  // Next midnight is +24 hours
  //NextMidnightTime = StartOfDayTime + 24ULL * 60 * 60;

  NextMidnightTime = StartOfDayTime + 3 * 60;
}