#include <Logme/LogmeC.h>

void LogStatisticsCWriteSiteA(int value)
{
  LogmeI_Ch("log_statistics_c", "c-site-a value=%d", value);
}

void LogStatisticsCWriteSiteB(const char* value)
{
  LogmeW_Ch("log_statistics_c", "c-site-b value=%s", value);
}
