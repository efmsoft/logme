/**
@file
@brief      Cross platform reenterable redefenitions for standard library time related functions
@details    Copyright (c) 2002-2015 Acronis
@author     Dmitry Komlev (dmitry.komlev@acronis.com)
@since      $Id: reentrant.h 3 2017-06-22 07:47:22Z eddy $
*/

#pragma once

#include <time.h>
#include <string.h>

#if defined(DJGPP) || defined(K_NATIVE)
#if !defined(K_NATIVE)
inline struct tm* gmtime_r(const time_t *timep, struct tm *result)
{
  struct tm* local_result = NULL;
  local_result = gmtime(timep);
  if (local_result != NULL)
  {
    *result = *local_result;
    return result;
  }
  return NULL;
}
#endif
inline struct tm* localtime_r(const time_t *timep, struct tm *result)
{
  struct tm* local_result = NULL;
  local_result = localtime(timep);
  if (local_result != NULL)
  {
    *result = *local_result;
    return result;
  }
  return NULL;
}

#define strtok_r(str, delim, saveptr) strtok(str, delim) // Ignore context pointer

#elif defined(_WIN32)

inline struct tm* gmtime_r(const time_t *timep, struct tm *result)
{
  return gmtime_s(result, timep) == 0 ? result : NULL;
}
inline struct tm* localtime_r(const time_t *timep, struct tm *result)
{
  return localtime_s(result, timep) == 0 ? result : NULL;
}

#define strtok_r strtok_s

#endif
