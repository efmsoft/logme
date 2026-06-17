#pragma once

#include <ctime>

#include <Logme/Backend/FileBackend.h>

namespace Logme
{
  class FileTimeRotationPolicy
  {
  public:
    LOGMELNK FileTimeRotationPolicy();

    LOGMELNK void Configure(TimeRotationMode mode);
    LOGMELNK void Configure(
      TimeRotationMode mode
      , std::time_t now
    );
    LOGMELNK TimeRotationMode GetMode() const;
    LOGMELNK const char* GetModeName() const;
    LOGMELNK bool IsEnabled() const;

    LOGMELNK std::time_t GetArchiveTime() const;
    LOGMELNK bool ShouldRotate(std::time_t& completedArchiveTime);
    LOGMELNK bool ShouldRotate(
      std::time_t now
      , std::time_t& completedArchiveTime
    );

  private:
    void Reset(std::time_t now);

    TimeRotationMode Mode;
    std::time_t PeriodStart;
    std::time_t NextPeriodStart;
  };
}
