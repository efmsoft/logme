#pragma once

#include <string>

#ifdef USE_ALLSTAT
#include <AllStat/AllStat.h>
#endif

#include <Logme/Types.h>

namespace Logme
{
  /// <summary>
  /// Converts a C runtime errno value to a readable error string.
  /// </summary>
  LOGMELNK std::string Errno2Str(int e);
  /// <summary>
  /// Converts a Windows GetLastError()/HRESULT-style value to a readable error string.
  /// </summary>
  LOGMELNK std::string Winerr2Str(int e);
}

/// <summary>
/// Converts an explicit errno value to a const char* for immediate use in Logme printf-style macros.
/// </summary>
#define ERRNO_STR(e) Logme::Errno2Str(e).c_str()
/// <summary>
/// Converts an explicit Windows error value to a const char* for immediate use in Logme printf-style macros.
/// </summary>
#define LRESULT_STR(e) Logme::Winerr2Str(e).c_str()

#ifdef _WIN32
/// <summary>
/// Converts an explicit operating-system error value to a const char* for immediate use in Logme
/// printf-style macros. On Windows this uses Winerr2Str().
/// </summary>
#define OSERR(e) LRESULT_STR(e)
/// <summary>
/// Converts the current operating-system error to a const char* for immediate use in Logme
/// printf-style macros. On Windows this reads GetLastError().
/// </summary>
#define OSERR2 LRESULT_STR(::GetLastError())
#else
/// <summary>
/// Converts an explicit operating-system error value to a const char* for immediate use in Logme
/// printf-style macros. On POSIX systems this uses Errno2Str().
/// </summary>
#define OSERR(e) ERRNO_STR(e)
/// <summary>
/// Converts the current operating-system error to a const char* for immediate use in Logme
/// printf-style macros. On POSIX systems this reads errno.
/// </summary>
#define OSERR2 ERRNO_STR(errno)
#endif
