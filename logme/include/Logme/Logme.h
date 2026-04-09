#pragma once

#include <Logme/AllStatI.h>
#include <Logme/AnsiColorEscape.h>
#include <Logme/ArgumentList.h>
#include <Logme/Channel.h>
#include <Logme/Convert.h>
#include <Logme/Logger.h>
#include <Logme/ObfString.h>
#include <Logme/Procedure.h>
#include <Logme/Stream.h>
#include <Logme/Template.h>
#include <Logme/ThreadChannel.h>
#include <Logme/ThreadName.h>
#include <Logme/ThreadOverride.h>
#include <utility>

// String conversion

#ifndef _S
#define _S(str) Logme::ToStdString(str)
#endif

#ifndef _WS
#define _WS(str) Logme::ToStdWString(str)
#endif

#include <Logme/Detail/Dispatch.h>

// C/C++ - style logging

#if _LOGME_ACTIVE
#ifdef _MSC_VER
  #define Logme_If(condition, logger, level, ...) \
    if (static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); (condition)) \
      Logme::Detail::Dispatch( \
        logger \
        , LOGME_JOIN(_logme_ctx_, __LINE__) \
        , level \
        , &CH \
        , &SUBSID \
        , __FUNCTION__ \
        , __FILE__ \
        , __LINE__ \
        , ## __VA_ARGS__ \
      )
  #define Logme_Ifg(condition, logger, level, ...) \
    if (static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); (condition)) \
      Logme::Detail::Dispatch( \
        logger \
        , LOGME_JOIN(_logme_ctx_, __LINE__) \
        , level \
        , &::CH \
        , &::SUBSID \
        , __FUNCTION__ \
        , __FILE__ \
        , __LINE__ \
        , ## __VA_ARGS__ \
      )
#else
  #define Logme_If(condition, logger, level, ...) \
    if (static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); (condition)) \
      Logme::Detail::Dispatch( \
        logger \
        , LOGME_JOIN(_logme_ctx_, __LINE__) \
        , level \
        , &CH \
        , &SUBSID \
        , __FUNCTION__ \
        , __FILE__ \
        , __LINE__ \
        __VA_OPT__(, __VA_ARGS__) \
      )
  #define Logme_Ifg(condition, logger, level, ...) \
    if (static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); (condition)) \
      Logme::Detail::Dispatch( \
        logger \
        , LOGME_JOIN(_logme_ctx_, __LINE__) \
        , level \
        , &::CH \
        , &::SUBSID \
        , __FUNCTION__ \
        , __FILE__ \
        , __LINE__ \
        __VA_OPT__(, __VA_ARGS__) \
      )
#endif
#else
  #define Logme_If(condition, logger, level, ...) if (true) { } else std::stringstream()
  #define Logme_Ifg(condition, logger, level, ...) if (true) { } else std::stringstream()
#endif

#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

#define LogmeD(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, ## __VA_ARGS__)

#define LogmeD_If(condition, ...) \
  Logme_If(condition, Logme::Instance, Logme::Level::LEVEL_DEBUG, ## __VA_ARGS__)

#define LogmeI(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, ## __VA_ARGS__)

#define LogmeI_If(condition, ...) \
  Logme_If(condition, Logme::Instance, Logme::Level::LEVEL_INFO, ## __VA_ARGS__)

#define LogmeW(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, ## __VA_ARGS__)

#define LogmeW_If(condition, ...) \
  Logme_If(condition, Logme::Instance, Logme::Level::LEVEL_WARN, ## __VA_ARGS__)

#define LogmeE(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, ## __VA_ARGS__)

#define LogmeE_If(condition, ...) \
  Logme_If(condition, Logme::Instance, Logme::Level::LEVEL_ERROR, ## __VA_ARGS__)

#define LogmeC(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, ## __VA_ARGS__)

#define LogmeC_If(condition, ...) \
  Logme_If(condition, Logme::Instance, Logme::Level::LEVEL_CRITICAL, ## __VA_ARGS__)

#define LogmeDg(...) \
  Logme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, ## __VA_ARGS__)

#define LogmeDg_If(condition, ...) \
  Logme_Ifg(condition, Logme::Instance, Logme::Level::LEVEL_DEBUG, ## __VA_ARGS__)

#define LogmeIg(...) \
  Logme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, ## __VA_ARGS__)

#define LogmeI_Ifg(condition, ...) \
  Logme_Ifg(condition, Logme::Instance, Logme::Level::LEVEL_INFO, ## __VA_ARGS__)

#define LogmeWg(...) \
  Logme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, ## __VA_ARGS__)

#define LogmeW_Ifg(condition, ...) \
  Logme_Ifg(condition, Logme::Instance, Logme::Level::LEVEL_WARN, ## __VA_ARGS__)

#define LogmeEg(...) \
  Logme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, ## __VA_ARGS__)

#define LogmeE_Ifg(condition, ...) \
  Logme_Ifg(condition, Logme::Instance, Logme::Level::LEVEL_ERROR, ## __VA_ARGS__)

#define LogmeCg(...) \
  Logme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, ## __VA_ARGS__)

#define LogmeC_Ifg(condition, ...) \
  Logme_Ifg(condition, Logme::Instance, Logme::Level::LEVEL_CRITICAL, ## __VA_ARGS__)

#if _LOGME_ACTIVE
  #define _LOGME_ONCE_OVR() ([]() -> Logme::Override& { static Logme::Override ovr(1); return ovr; }())
  #define _LOGME_RATE_OVR(ms) ([]() -> Logme::Override& { static Logme::Override ovr(-1, ms); return ovr; }())
#else
  #define _LOGME_ONCE_OVR()
  #define _LOGME_RATE_OVR(ms)
#endif

// Log once (per call site)

#ifdef _MSC_VER
#define LogmeD_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, _LOGME_ONCE_OVR(), ## __VA_ARGS__)
#else
#define LogmeD_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, _LOGME_ONCE_OVR() _LOGME_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
#define LogmeI_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, _LOGME_ONCE_OVR(), ## __VA_ARGS__)
#else
#define LogmeI_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, _LOGME_ONCE_OVR() _LOGME_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
#define LogmeW_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, _LOGME_ONCE_OVR(), ## __VA_ARGS__)
#else
#define LogmeW_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, _LOGME_ONCE_OVR() _LOGME_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
#define LogmeE_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, _LOGME_ONCE_OVR(), ## __VA_ARGS__)
#else
#define LogmeE_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, _LOGME_ONCE_OVR() _LOGME_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
#define LogmeC_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, _LOGME_ONCE_OVR(), ## __VA_ARGS__)
#else
#define LogmeC_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, _LOGME_ONCE_OVR() _LOGME_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

// Log with interval (rate-limited)

#ifdef _MSC_VER
#define LogmeD_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, _LOGME_RATE_OVR(ms), ## __VA_ARGS__)
#else
#define LogmeD_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, _LOGME_RATE_OVR(ms) _LOGME_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
#define LogmeI_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, _LOGME_RATE_OVR(ms), ## __VA_ARGS__)
#else
#define LogmeI_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, _LOGME_RATE_OVR(ms) _LOGME_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
#define LogmeW_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, _LOGME_RATE_OVR(ms), ## __VA_ARGS__)
#else
#define LogmeW_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, _LOGME_RATE_OVR(ms) _LOGME_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
#define LogmeE_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, _LOGME_RATE_OVR(ms), ## __VA_ARGS__)
#else
#define LogmeE_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, _LOGME_RATE_OVR(ms) _LOGME_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
#define LogmeC_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, _LOGME_RATE_OVR(ms), ## __VA_ARGS__)
#else
#define LogmeC_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, _LOGME_RATE_OVR(ms) _LOGME_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

// std::format

#if _LOGME_ACTIVE
#ifdef _MSC_VER
  #define fLogme_If(condition, logger, level, ...) \
    do \
    { \
      if ((condition)) \
      { \
        LOGME_PRAGMA_PUSH \
        LOGME_PRAGMA_IGNORE_VARARGS \
        static Logme::ContextCache _logme_ctx_; \
        Logme::Detail::DispatchStdFormat( \
          logger \
          , _logme_ctx_ \
          , level \
          , &CH \
          , &SUBSID \
          , __FUNCTION__ \
          , __FILE__ \
          , __LINE__ \
          , ## __VA_ARGS__ \
        ); \
        LOGME_PRAGMA_POP \
      } \
    } while (0)
  #define fLogme_Ifg(condition, logger, level, ...) \
    do \
    { \
      if ((condition)) \
      { \
        LOGME_PRAGMA_PUSH \
        LOGME_PRAGMA_IGNORE_VARARGS \
        static Logme::ContextCache _logme_ctx_; \
        Logme::Detail::DispatchStdFormat( \
          logger \
          , _logme_ctx_ \
          , level \
          , &::CH \
          , &SUBSID \
          , __FUNCTION__ \
          , __FILE__ \
          , __LINE__ \
          , ## __VA_ARGS__ \
        ); \
        LOGME_PRAGMA_POP \
      } \
    } while (0)
#else
  #define fLogme_If(condition, logger, level, ...)     do {       if ((condition)) {         LOGME_PRAGMA_PUSH         LOGME_PRAGMA_IGNORE_VARARGS         static Logme::ContextCache _logme_ctx_;         Logme::Detail::DispatchStdFormat(           logger           , _logme_ctx_           , level           , &CH           , &SUBSID           , __FUNCTION__           , __FILE__           , __LINE__           __VA_OPT__(, __VA_ARGS__)         );         LOGME_PRAGMA_POP       }     } while (0)
  #define fLogme_Ifg(condition, logger, level, ...)     do {       if ((condition)) {         LOGME_PRAGMA_PUSH         LOGME_PRAGMA_IGNORE_VARARGS         static Logme::ContextCache _logme_ctx_;         Logme::Detail::DispatchStdFormat(           logger           , _logme_ctx_           , level           , &::CH           , &::SUBSID           , __FUNCTION__           , __FILE__           , __LINE__           __VA_OPT__(, __VA_ARGS__)         );         LOGME_PRAGMA_POP       }     } while (0)
#endif
#else
  #define fLogme_If(condition, logger, level, ...) do { } while (0)
  #define fLogme_Ifg(condition, logger, level, ...) do { } while (0)
#endif

#ifndef LOGME_DISABLE_STD_FORMAT

#define fLogmeD(...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeD_If(condition, ...) \
  fLogme_If(condition, Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeI(...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeI_If(condition, ...) \
  fLogme_If(condition, Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeW(...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeW_If(condition, ...) \
  fLogme_If(condition, Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeE(...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeE_If(condition, ...) \
  fLogme_If(condition, Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeC(...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeC_If(condition, ...) \
  fLogme_If(condition, Logme::Instance, Logme::Level::LEVEL_CRITICAL, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeDg(...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeDg_If(condition, ...) \
  fLogme_Ifg(condition, Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeIg(...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeI_Ifg(condition, ...) \
  fLogme_Ifg(condition, Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeWg(...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeW_Ifg(condition, ...) \
  fLogme_Ifg(condition, Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeEg(...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeE_Ifg(condition, ...) \
  fLogme_Ifg(condition, Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeD_Once(...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), _LOGME_ONCE_OVR(), ## __VA_ARGS__)

#define fLogmeI_Once(...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), _LOGME_ONCE_OVR(), ## __VA_ARGS__)

#define fLogmeW_Once(...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), _LOGME_ONCE_OVR(), ## __VA_ARGS__)

#define fLogmeE_Once(...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), _LOGME_ONCE_OVR(), ## __VA_ARGS__)

#define fLogmeC_Once(...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, Logme::GetStdFormat(), _LOGME_ONCE_OVR(), ## __VA_ARGS__)

#define fLogmeDg_Once(...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), _LOGME_ONCE_OVR(), ## __VA_ARGS__)

#define fLogmeIg_Once(...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), _LOGME_ONCE_OVR(), ## __VA_ARGS__)

#define fLogmeWg_Once(...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), _LOGME_ONCE_OVR(), ## __VA_ARGS__)

#define fLogmeEg_Once(...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), _LOGME_ONCE_OVR(), ## __VA_ARGS__)

#define fLogmeCg_Once(...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, Logme::GetStdFormat(), _LOGME_ONCE_OVR(), ## __VA_ARGS__)

#define fLogmeD_Every(ms, ...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), _LOGME_RATE_OVR(ms), ## __VA_ARGS__)

#define fLogmeI_Every(ms, ...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), _LOGME_RATE_OVR(ms), ## __VA_ARGS__)

#define fLogmeW_Every(ms, ...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), _LOGME_RATE_OVR(ms), ## __VA_ARGS__)

#define fLogmeE_Every(ms, ...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), _LOGME_RATE_OVR(ms), ## __VA_ARGS__)

#define fLogmeC_Every(ms, ...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, Logme::GetStdFormat(), _LOGME_RATE_OVR(ms), ## __VA_ARGS__)

#define fLogmeDg_Every(ms, ...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), _LOGME_RATE_OVR(ms), ## __VA_ARGS__)

#define fLogmeIg_Every(ms, ...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), _LOGME_RATE_OVR(ms), ## __VA_ARGS__)

#define fLogmeWg_Every(ms, ...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), _LOGME_RATE_OVR(ms), ## __VA_ARGS__)

#define fLogmeEg_Every(ms, ...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), _LOGME_RATE_OVR(ms), ## __VA_ARGS__)

#define fLogmeCg_Every(ms, ...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, Logme::GetStdFormat(), _LOGME_RATE_OVR(ms), ## __VA_ARGS__)

#else

#define fLogmeC(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)

#define fLogmeC_If(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)

#define fLogmeC_Ifg(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)

#define fLogmeCg(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)

#define fLogmeD(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)

#define fLogmeD_If(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)

#define fLogmeDg(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)

#define fLogmeDg_If(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)

#define fLogmeE(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)

#define fLogmeE_If(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)

#define fLogmeE_Ifg(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)

#define fLogmeEg(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)

#define fLogmeI(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)

#define fLogmeI_If(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)

#define fLogmeI_Ifg(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)

#define fLogmeIg(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)

#define fLogmeW(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)

#define fLogmeW_If(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)

#define fLogmeW_Ifg(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)

#define fLogmeWg(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)

#endif

#if defined(__clang__)
  #pragma clang diagnostic pop
#endif

// Thread channel 

#define LogmeThreadChannel(ch) \
  Logme::ThreadChannel _logme_thread_channel(Logme::Instance, ch)

// Thread name

#define LogmeThreadName(pch, ...) \
  Logme::ThreadName _logme_thread_name(pch, __VA_ARGS__)

#define LogmeThreadChannelDefined() \
  Logme::Instance->IsChannelDefinedForCurrentThread()

// Thread override

#define LogmeThreadOverride(ovr) \
  Logme::ThreadOverride _logme_thread_override(Logme::Instance, ovr)
