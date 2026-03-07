#ifndef _WIN32

#include <cstdarg>
#include <cstdio>
#include <cstring>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#pragma clang diagnostic ignored "-Wformat-security"
#endif

inline void strcpy_s(
  char* dst
  , size_t n
  , const char* src
)
{
  if (dst == nullptr || n == 0)
    return;

  if (src == nullptr)
  {
    dst[0] = '\0';
    return;
  }

  size_t len = std::strlen(src);

  if (len >= n)
  {
    std::memcpy(dst, src, n - 1);
    dst[n - 1] = '\0';
  }
  else
  {
    std::memcpy(dst, src, len + 1);
  }
}

inline int sprintf_s(
  char* dst
  , size_t n
  , const char* format
  , ...
)
{
  if (dst == nullptr || n == 0 || format == nullptr)
    return -1;

  va_list args;
  va_start(args, format);
  int rc = std::vsnprintf(dst, n, format, args);
  va_end(args);

  if (rc < 0)
  {
    dst[0] = '\0';
    return -1;
  }

  if ((size_t)rc >= n)
  {
    dst[n - 1] = '\0';
    return -1;
  }

  return rc;
}

inline int vsprintf_s(
  char* dst
  , size_t n
  , const char* format
  , va_list args
)
{
  if (dst == nullptr || n == 0 || format == nullptr)
    return -1;

  int rc = std::vsnprintf(dst, n, format, args);

  if (rc < 0)
  {
    dst[0] = '\0';
    return -1;
  }

  if ((size_t)rc >= n)
  {
    dst[n - 1] = '\0';
    return -1;
  }

  return rc;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif