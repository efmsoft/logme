#pragma once

#include <Logme/Time/types.h>

namespace Logme
{
#ifdef _WIN32
  /**
  * @brief convert Unix time (Seconds since 1970) to SystemTime
  * @details uses RtlSecondsSince1970ToTime from Ntdll.dll if possible
  * @param time Unix time
  * @param stime SystemTime
  * @return true indicates sucess, false indicates failure
  */
  bool UnixTimeToSystemTime(const uni_time_t& time, SystemTime& stime);

  class DateTime;

  /**
  * @brief convert SystemTime to Unix time (Seconds since 1970)
  * @details uses RtlTimeToSecondsSince1970 from Ntdll.dll if possible
  * @param stime SystemTime
  * @param time Unix time
  * @return true indicates success, false indicates failure
  */
  bool SystemTimeToUnixTime(const SystemTime& stime, uni_time_t& time);

  /**
  * @brief convert DateTime to SystemTime
  * @param time DateTime
  * @param stime SystemTime
  * @return true indicates success, false indicates failure
  */
  bool DateTimeToSystemTime(const DateTime& time, SystemTime& stime);
#endif
}
