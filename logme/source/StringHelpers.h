#pragma once
#ifndef _WIN32

inline void strcpy_s(char* dst, size_t n, const char* src)
{
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
  va_list args;
  va_start(args, format);

  int rc = vsprintf(dst, format, args);

  va_end(args);
  return rc;
}

#endif
