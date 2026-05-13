#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <Logme/OutputFlags.h>

#define LOGMEC_API LOGMELNK

#ifndef LOGME_ACTIVE
  #if !defined(_DEBUG) && !defined(LOGME_INRELEASE)
    #define LOGME_ACTIVE 0
  #else
    #define LOGME_ACTIVE 1
  #endif
#endif

#ifdef __cplusplus
typedef Logme::Level LogmeLevel;
typedef Logme::OutputFlags LogmeOutputFlags;
#else
typedef enum Level LogmeLevel;
typedef union OutputFlags LogmeOutputFlags;
#endif

#ifdef __cplusplus
extern "C"
{
#endif




typedef struct LogmeCShortenerPair
{
  const char* SearchFor;
  const char* ReplaceOn;
} LogmeCShortenerPair;

typedef struct LogmeCOverride
{
  LogmeOutputFlags Add;
  LogmeOutputFlags Remove;
  int MaxRepetitions;
  int Repetitions;
  uint64_t MaxFrequency;
  uint64_t LastTime;
  const LogmeCShortenerPair* Shortener;
} LogmeCOverride;

#define LOGME_C_OVERRIDE_INIT { { 0 }, { 0 }, -1, 0, 0, 0, NULL }

LOGMEC_API void LogmeWriteV(
  LogmeLevel level
  , const char* channel
  , const char* subsystem
  , const char* function
  , const char* file
  , int line
  , const char* format
  , va_list args
);


LOGMEC_API void LogmeWriteOverrideV(
  LogmeLevel level
  , const char* channel
  , const char* subsystem
  , LogmeCOverride* overrideData
  , const char* function
  , const char* file
  , int line
  , const char* format
  , va_list args
);

LOGMEC_API void LogmeWrite(
  LogmeLevel level
  , const char* channel
  , const char* subsystem
  , const char* function
  , const char* file
  , int line
  , const char* format
  , ...
);


LOGMEC_API void LogmeWriteOverride(
  LogmeLevel level
  , const char* channel
  , const char* subsystem
  , LogmeCOverride* overrideData
  , const char* function
  , const char* file
  , int line
  , const char* format
  , ...
);

LOGMEC_API void LogmeInitOverride(
  LogmeCOverride* overrideData
  , int maxRepetitions
  , uint64_t maxFrequency
);

LOGMEC_API int LogmeLoadConfigurationFile(
  const char* configFile
  , const char* section
  , char* errorBuffer
  , size_t errorBufferSize
);

LOGMEC_API int LogmeLoadConfiguration(
  const char* configData
  , const char* section
  , char* errorBuffer
  , size_t errorBufferSize
);

LOGMEC_API void LogmeShutdown(void);

LOGMEC_API void LogmeSetChannelLevel(
  const char* channel
  , LogmeLevel level
);

LOGMEC_API void LogmeSetChannelEnabled(
  const char* channel
  , int enabled
);

LOGMEC_API int LogmeCreateChannel(
  const char* channel
  , LogmeLevel level
);

LOGMEC_API void LogmeDeleteChannel(
  const char* channel
);

LOGMEC_API void LogmeRemoveChannelBackends(
  const char* channel
);

LOGMEC_API void LogmeSetChannelFlags(
  const char* channel
  , LogmeOutputFlags flags
);

LOGMEC_API int LogmeAddConsoleBackend(
  const char* channel
  , int async
);

LOGMEC_API int LogmeAddDebugBackend(
  const char* channel
);

LOGMEC_API int LogmeAddFileBackend(
  const char* channel
  , const char* fileName
  , int append
  , size_t maxSize
  , int dailyRotation
  , int maxParts
);

LOGMEC_API void LogmeFlushChannel(
  const char* channel
);

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
  #define LOGME_C_ENUM_VALUE(name) Logme::name
#else
  #define LOGME_C_ENUM_VALUE(name) name
#endif

#ifndef LOGME_C_FUNCTION
  #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    #define LOGME_C_FUNCTION __func__
  #elif defined(_MSC_VER)
    #define LOGME_C_FUNCTION __FUNCTION__
  #else
    #define LOGME_C_FUNCTION ""
  #endif
#endif

#if LOGME_ACTIVE
  #if defined(_MSC_VER)
    #define LogmeD(...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_DEBUG), NULL, NULL, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeI(...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_INFO), NULL, NULL, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeW(...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_WARN), NULL, NULL, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeE(...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_ERROR), NULL, NULL, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeC(...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_CRITICAL), NULL, NULL, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)

    #define LogmeD_Ch(channel, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_DEBUG), channel, NULL, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeI_Ch(channel, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_INFO), channel, NULL, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeW_Ch(channel, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_WARN), channel, NULL, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeE_Ch(channel, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_ERROR), channel, NULL, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeC_Ch(channel, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_CRITICAL), channel, NULL, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)

    #define LogmeD_Sid(subsystem, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_DEBUG), NULL, subsystem, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeI_Sid(subsystem, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_INFO), NULL, subsystem, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeW_Sid(subsystem, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_WARN), NULL, subsystem, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeE_Sid(subsystem, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_ERROR), NULL, subsystem, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeC_Sid(subsystem, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_CRITICAL), NULL, subsystem, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)

    #define LogmeD_ChSid(channel, subsystem, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_DEBUG), channel, subsystem, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeI_ChSid(channel, subsystem, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_INFO), channel, subsystem, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeW_ChSid(channel, subsystem, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_WARN), channel, subsystem, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeE_ChSid(channel, subsystem, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_ERROR), channel, subsystem, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeC_ChSid(channel, subsystem, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_CRITICAL), channel, subsystem, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)

    #define LogmeD_Ovr(overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_DEBUG), NULL, NULL, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeI_Ovr(overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_INFO), NULL, NULL, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeW_Ovr(overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_WARN), NULL, NULL, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeE_Ovr(overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_ERROR), NULL, NULL, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeC_Ovr(overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_CRITICAL), NULL, NULL, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)

    #define LogmeD_ChOvr(channel, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_DEBUG), channel, NULL, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeI_ChOvr(channel, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_INFO), channel, NULL, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeW_ChOvr(channel, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_WARN), channel, NULL, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeE_ChOvr(channel, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_ERROR), channel, NULL, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeC_ChOvr(channel, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_CRITICAL), channel, NULL, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)

    #define LogmeD_SidOvr(subsystem, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_DEBUG), NULL, subsystem, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeI_SidOvr(subsystem, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_INFO), NULL, subsystem, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeW_SidOvr(subsystem, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_WARN), NULL, subsystem, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeE_SidOvr(subsystem, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_ERROR), NULL, subsystem, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeC_SidOvr(subsystem, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_CRITICAL), NULL, subsystem, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)

    #define LogmeD_ChSidOvr(channel, subsystem, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_DEBUG), channel, subsystem, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeI_ChSidOvr(channel, subsystem, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_INFO), channel, subsystem, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeW_ChSidOvr(channel, subsystem, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_WARN), channel, subsystem, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeE_ChSidOvr(channel, subsystem, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_ERROR), channel, subsystem, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeC_ChSidOvr(channel, subsystem, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_CRITICAL), channel, subsystem, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
  #else
    #define LogmeD(...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_DEBUG), NULL, NULL, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeI(...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_INFO), NULL, NULL, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeW(...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_WARN), NULL, NULL, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeE(...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_ERROR), NULL, NULL, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeC(...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_CRITICAL), NULL, NULL, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)

    #define LogmeD_Ch(channel, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_DEBUG), channel, NULL, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeI_Ch(channel, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_INFO), channel, NULL, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeW_Ch(channel, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_WARN), channel, NULL, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeE_Ch(channel, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_ERROR), channel, NULL, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeC_Ch(channel, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_CRITICAL), channel, NULL, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)

    #define LogmeD_Sid(subsystem, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_DEBUG), NULL, subsystem, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeI_Sid(subsystem, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_INFO), NULL, subsystem, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeW_Sid(subsystem, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_WARN), NULL, subsystem, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeE_Sid(subsystem, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_ERROR), NULL, subsystem, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeC_Sid(subsystem, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_CRITICAL), NULL, subsystem, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)

    #define LogmeD_ChSid(channel, subsystem, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_DEBUG), channel, subsystem, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeI_ChSid(channel, subsystem, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_INFO), channel, subsystem, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeW_ChSid(channel, subsystem, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_WARN), channel, subsystem, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeE_ChSid(channel, subsystem, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_ERROR), channel, subsystem, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeC_ChSid(channel, subsystem, ...) LogmeWrite(LOGME_C_ENUM_VALUE(LEVEL_CRITICAL), channel, subsystem, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)

    #define LogmeD_Ovr(overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_DEBUG), NULL, NULL, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeI_Ovr(overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_INFO), NULL, NULL, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeW_Ovr(overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_WARN), NULL, NULL, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeE_Ovr(overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_ERROR), NULL, NULL, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeC_Ovr(overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_CRITICAL), NULL, NULL, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)

    #define LogmeD_ChOvr(channel, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_DEBUG), channel, NULL, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeI_ChOvr(channel, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_INFO), channel, NULL, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeW_ChOvr(channel, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_WARN), channel, NULL, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeE_ChOvr(channel, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_ERROR), channel, NULL, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeC_ChOvr(channel, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_CRITICAL), channel, NULL, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)

    #define LogmeD_SidOvr(subsystem, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_DEBUG), NULL, subsystem, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeI_SidOvr(subsystem, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_INFO), NULL, subsystem, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeW_SidOvr(subsystem, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_WARN), NULL, subsystem, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeE_SidOvr(subsystem, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_ERROR), NULL, subsystem, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeC_SidOvr(subsystem, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_CRITICAL), NULL, subsystem, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)

    #define LogmeD_ChSidOvr(channel, subsystem, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_DEBUG), channel, subsystem, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeI_ChSidOvr(channel, subsystem, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_INFO), channel, subsystem, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeW_ChSidOvr(channel, subsystem, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_WARN), channel, subsystem, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeE_ChSidOvr(channel, subsystem, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_ERROR), channel, subsystem, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
    #define LogmeC_ChSidOvr(channel, subsystem, overrideData, ...) LogmeWriteOverride(LOGME_C_ENUM_VALUE(LEVEL_CRITICAL), channel, subsystem, overrideData, LOGME_C_FUNCTION, __FILE__, __LINE__, ## __VA_ARGS__)
  #endif

  #define LogmeD_If(condition, ...) do { if (condition) { LogmeD(__VA_ARGS__); } } while (0)
  #define LogmeI_If(condition, ...) do { if (condition) { LogmeI(__VA_ARGS__); } } while (0)
  #define LogmeW_If(condition, ...) do { if (condition) { LogmeW(__VA_ARGS__); } } while (0)
  #define LogmeE_If(condition, ...) do { if (condition) { LogmeE(__VA_ARGS__); } } while (0)
  #define LogmeC_If(condition, ...) do { if (condition) { LogmeC(__VA_ARGS__); } } while (0)
#else
  #define LogmeD(...) do { } while (0)
  #define LogmeI(...) do { } while (0)
  #define LogmeW(...) do { } while (0)
  #define LogmeE(...) do { } while (0)
  #define LogmeC(...) do { } while (0)

  #define LogmeD_Ch(channel, ...) do { } while (0)
  #define LogmeI_Ch(channel, ...) do { } while (0)
  #define LogmeW_Ch(channel, ...) do { } while (0)
  #define LogmeE_Ch(channel, ...) do { } while (0)
  #define LogmeC_Ch(channel, ...) do { } while (0)

  #define LogmeD_Sid(subsystem, ...) do { } while (0)
  #define LogmeI_Sid(subsystem, ...) do { } while (0)
  #define LogmeW_Sid(subsystem, ...) do { } while (0)
  #define LogmeE_Sid(subsystem, ...) do { } while (0)
  #define LogmeC_Sid(subsystem, ...) do { } while (0)

  #define LogmeD_ChSid(channel, subsystem, ...) do { } while (0)
  #define LogmeI_ChSid(channel, subsystem, ...) do { } while (0)
  #define LogmeW_ChSid(channel, subsystem, ...) do { } while (0)
  #define LogmeE_ChSid(channel, subsystem, ...) do { } while (0)
  #define LogmeC_ChSid(channel, subsystem, ...) do { } while (0)

  #define LogmeD_Ovr(overrideData, ...) do { } while (0)
  #define LogmeI_Ovr(overrideData, ...) do { } while (0)
  #define LogmeW_Ovr(overrideData, ...) do { } while (0)
  #define LogmeE_Ovr(overrideData, ...) do { } while (0)
  #define LogmeC_Ovr(overrideData, ...) do { } while (0)

  #define LogmeD_ChOvr(channel, overrideData, ...) do { } while (0)
  #define LogmeI_ChOvr(channel, overrideData, ...) do { } while (0)
  #define LogmeW_ChOvr(channel, overrideData, ...) do { } while (0)
  #define LogmeE_ChOvr(channel, overrideData, ...) do { } while (0)
  #define LogmeC_ChOvr(channel, overrideData, ...) do { } while (0)

  #define LogmeD_SidOvr(subsystem, overrideData, ...) do { } while (0)
  #define LogmeI_SidOvr(subsystem, overrideData, ...) do { } while (0)
  #define LogmeW_SidOvr(subsystem, overrideData, ...) do { } while (0)
  #define LogmeE_SidOvr(subsystem, overrideData, ...) do { } while (0)
  #define LogmeC_SidOvr(subsystem, overrideData, ...) do { } while (0)

  #define LogmeD_ChSidOvr(channel, subsystem, overrideData, ...) do { } while (0)
  #define LogmeI_ChSidOvr(channel, subsystem, overrideData, ...) do { } while (0)
  #define LogmeW_ChSidOvr(channel, subsystem, overrideData, ...) do { } while (0)
  #define LogmeE_ChSidOvr(channel, subsystem, overrideData, ...) do { } while (0)
  #define LogmeC_ChSidOvr(channel, subsystem, overrideData, ...) do { } while (0)

  #define LogmeD_If(condition, ...) do { } while (0)
  #define LogmeI_If(condition, ...) do { } while (0)
  #define LogmeW_If(condition, ...) do { } while (0)
  #define LogmeE_If(condition, ...) do { } while (0)
  #define LogmeC_If(condition, ...) do { } while (0)
#endif
