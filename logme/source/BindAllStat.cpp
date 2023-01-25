#include <Logme/Logme.h>

std::string ErrnoStr(int e)
{
#ifdef USE_ALLSTAT
  return AllStat::Errno2Str(e);
#else
  return std::to_string(e);
#endif
}

std::string LresultStr(int e)
{
#ifdef USE_ALLSTAT
  return AllStat::Winerr2Str(e);
#else
  return std::to_string(e);
#endif
}
