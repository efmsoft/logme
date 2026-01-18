#pragma once
#ifndef _WIN32

#include <stdarg.h>
#include <string.h>

#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

inline void strcpy_s(char* dst, size_t n, const char* src)
{
  (void)n;
  strcpy(dst, src);
}

inline int sprintf_s(char* dst, const char* format, ...)
{
  va_list args;
  va_start(args, format);

  int rc = vsprintf(dst, format, args);

  va_end(args);
  return rc;
}

inline int sprintf_s(char* dst, size_t n, const char* format, ...)
{
  (void)n;

  va_list args;
  va_start(args, format);

  int rc = vsprintf(dst, format, args);

  va_end(args);
  return rc;
}

#if defined(__clang__)
  #pragma clang diagnostic pop
#endif

#endif
