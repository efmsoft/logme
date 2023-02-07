#include <Logme/Logme.h>

std::string Logme::Errno2Str(int e)
{
#ifdef USE_ALLSTAT
  return AllStat::Errno2Str(e);
#else
  return std::to_string(e);
#endif
}

std::string Logme::Winerr2Str(int e)
{
#ifdef USE_ALLSTAT
  return AllStat::Winerr2Str(e);
#else
  return std::to_string(e);
#endif
}
