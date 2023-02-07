#pragma once

#include <string>

#ifdef USE_ALLSTAT
#include <AllStat/AllStat.h>
#endif

namespace Logme
{
  std::string Errno2Str(int e);
  std::string Winerr2Str(int e);
}

#define ERRNO_STR(e) Logme::Errno2Str(e).c_str()
#define LRESULT_STR(e) Logme::Winerr2Str(e).c_str()

#ifdef _WIN32
#define OSERR(e) LRESULT_STR(e)
#define OSERR2 LRESULT_STR(::GetLastError())
#else
#define OSERR(e) ERRNO_STR_STR(e)
#define OSERR2 ERRNO_STR_STR(errno)
#endif
