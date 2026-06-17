#include "FileTimeRotationPolicy.h"

#include <ctime>

namespace
{
  bool GetLocalTime(
    std::time_t value
    , struct tm& tmValue
  )
  {
#ifdef _WIN32
    return localtime_s(&tmValue, &value) == 0;
#else
    return localtime_r(&value, &tmValue) != nullptr;
#endif
  }

  std::time_t MakeTime(struct tm& tmValue)
  {
    tmValue.tm_isdst = -1;
    return mktime(&tmValue);
  }

  std::time_t SafeNextTime(
    std::time_t start
    , std::time_t fallbackPeriodSeconds
  )
  {
    if (start == static_cast<std::time_t>(-1))
    {
      return static_cast<std::time_t>(-1);
    }

    return start + fallbackPeriodSeconds;
  }

  std::time_t BuildPeriodStart(
    Logme::TimeRotationMode mode
    , std::time_t now
  )
  {
    struct tm tmValue {};
    if (!GetLocalTime(now, tmValue))
    {
      return now;
    }

    if (mode == Logme::TIME_ROTATION_HOURLY)
    {
      tmValue.tm_min = 0;
      tmValue.tm_sec = 0;
      return MakeTime(tmValue);
    }

    tmValue.tm_hour = 0;
    tmValue.tm_min = 0;
    tmValue.tm_sec = 0;

    if (mode == Logme::TIME_ROTATION_WEEKLY)
    {
      const int daysFromMonday = (tmValue.tm_wday + 6) % 7;
      tmValue.tm_mday -= daysFromMonday;
      return MakeTime(tmValue);
    }

    if (mode == Logme::TIME_ROTATION_MONTHLY)
    {
      tmValue.tm_mday = 1;
      return MakeTime(tmValue);
    }

    return MakeTime(tmValue);
  }

  std::time_t BuildNextPeriodStart(
    Logme::TimeRotationMode mode
    , std::time_t periodStart
  )
  {
    struct tm tmValue {};
    if (!GetLocalTime(periodStart, tmValue))
    {
      if (mode == Logme::TIME_ROTATION_HOURLY)
        return SafeNextTime(periodStart, 60 * 60);
      if (mode == Logme::TIME_ROTATION_WEEKLY)
        return SafeNextTime(periodStart, 7 * 24 * 60 * 60);
      return SafeNextTime(periodStart, 24 * 60 * 60);
    }

    if (mode == Logme::TIME_ROTATION_HOURLY)
    {
      tmValue.tm_hour += 1;
    }
    else if (mode == Logme::TIME_ROTATION_WEEKLY)
    {
      tmValue.tm_mday += 7;
    }
    else if (mode == Logme::TIME_ROTATION_MONTHLY)
    {
      tmValue.tm_mon += 1;
    }
    else
    {
      tmValue.tm_mday += 1;
    }

    return MakeTime(tmValue);
  }
}

namespace Logme
{
  FileTimeRotationPolicy::FileTimeRotationPolicy()
    : Mode(TIME_ROTATION_NONE)
    , PeriodStart(0)
    , NextPeriodStart(0)
  {
  }

  void FileTimeRotationPolicy::Configure(TimeRotationMode mode)
  {
    Configure(mode, time(nullptr));
  }

  void FileTimeRotationPolicy::Configure(
    TimeRotationMode mode
    , std::time_t now
  )
  {
    Mode = mode;
    Reset(now);
  }

  TimeRotationMode FileTimeRotationPolicy::GetMode() const
  {
    return Mode;
  }

  const char* FileTimeRotationPolicy::GetModeName() const
  {
    if (Mode == TIME_ROTATION_HOURLY)
      return "hourly";
    if (Mode == TIME_ROTATION_DAILY)
      return "daily";
    if (Mode == TIME_ROTATION_WEEKLY)
      return "weekly";
    if (Mode == TIME_ROTATION_MONTHLY)
      return "monthly";

    return "none";
  }

  bool FileTimeRotationPolicy::IsEnabled() const
  {
    return Mode != TIME_ROTATION_NONE;
  }

  std::time_t FileTimeRotationPolicy::GetArchiveTime() const
  {
    if (IsEnabled() && PeriodStart != 0)
      return PeriodStart;

    return time(nullptr);
  }

  bool FileTimeRotationPolicy::ShouldRotate(std::time_t& completedArchiveTime)
  {
    if (!IsEnabled())
      return false;

    return ShouldRotate(time(nullptr), completedArchiveTime);
  }

  bool FileTimeRotationPolicy::ShouldRotate(
    std::time_t now
    , std::time_t& completedArchiveTime
  )
  {
    if (!IsEnabled())
      return false;

    if (NextPeriodStart == 0)
    {
      Reset(now);
      return false;
    }

    if (now < NextPeriodStart)
      return false;

    completedArchiveTime = PeriodStart != 0 ? PeriodStart : now;
    Reset(now);
    return true;
  }

  void FileTimeRotationPolicy::Reset(std::time_t now)
  {
    if (!IsEnabled())
    {
      PeriodStart = 0;
      NextPeriodStart = 0;
      return;
    }

    PeriodStart = BuildPeriodStart(Mode, now);
    if (PeriodStart == static_cast<std::time_t>(-1))
      PeriodStart = now;

    NextPeriodStart = BuildNextPeriodStart(Mode, PeriodStart);
    if (NextPeriodStart == static_cast<std::time_t>(-1) || NextPeriodStart <= PeriodStart)
    {
      if (Mode == TIME_ROTATION_HOURLY)
        NextPeriodStart = PeriodStart + 60 * 60;
      else if (Mode == TIME_ROTATION_WEEKLY)
        NextPeriodStart = PeriodStart + 7 * 24 * 60 * 60;
      else
        NextPeriodStart = PeriodStart + 24 * 60 * 60;
    }
  }
}
