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
#include <Logme/Detail/Precheck.h>

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

// Public logging macros accept the same optional argument set:
//   [channel/id or ChannelPtr], [Override], [SID], format, args...
// Override may appear before or after channel where supported by Logger overloads.

/// <summary>
/// Writes DEBUG log message (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeD(...) \
  Logme_If(Logme::Instance->Condition() && LOGME_WOULD_LOG_FIRST(Logme::Instance.get(), Logme::Level::LEVEL_DEBUG, ## __VA_ARGS__), Logme::Instance, Logme::Level::LEVEL_DEBUG, ## __VA_ARGS__)

/// <summary>
/// Writes DEBUG log message when condition is true (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="condition">Expression checked before logging.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeD_If(condition, ...) \
  Logme_If(condition, Logme::Instance, Logme::Level::LEVEL_DEBUG, ## __VA_ARGS__)

/// <summary>
/// Writes INFO log message (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeI(...) \
  Logme_If(Logme::Instance->Condition() && LOGME_WOULD_LOG_FIRST(Logme::Instance.get(), Logme::Level::LEVEL_INFO, ## __VA_ARGS__), Logme::Instance, Logme::Level::LEVEL_INFO, ## __VA_ARGS__)

/// <summary>
/// Writes INFO log message when condition is true (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="condition">Expression checked before logging.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeI_If(condition, ...) \
  Logme_If(condition, Logme::Instance, Logme::Level::LEVEL_INFO, ## __VA_ARGS__)

/// <summary>
/// Writes WARN log message (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeW(...) \
  Logme_If(Logme::Instance->Condition() && LOGME_WOULD_LOG_FIRST(Logme::Instance.get(), Logme::Level::LEVEL_WARN, ## __VA_ARGS__), Logme::Instance, Logme::Level::LEVEL_WARN, ## __VA_ARGS__)

/// <summary>
/// Writes WARN log message when condition is true (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="condition">Expression checked before logging.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeW_If(condition, ...) \
  Logme_If(condition, Logme::Instance, Logme::Level::LEVEL_WARN, ## __VA_ARGS__)

/// <summary>
/// Writes ERROR log message (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeE(...) \
  Logme_If(Logme::Instance->Condition() && LOGME_WOULD_LOG_FIRST(Logme::Instance.get(), Logme::Level::LEVEL_ERROR, ## __VA_ARGS__), Logme::Instance, Logme::Level::LEVEL_ERROR, ## __VA_ARGS__)

/// <summary>
/// Writes ERROR log message when condition is true (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="condition">Expression checked before logging.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeE_If(condition, ...) \
  Logme_If(condition, Logme::Instance, Logme::Level::LEVEL_ERROR, ## __VA_ARGS__)

/// <summary>
/// Writes CRITICAL log message (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeC(...) \
  Logme_If(Logme::Instance->Condition() && LOGME_WOULD_LOG_FIRST(Logme::Instance.get(), Logme::Level::LEVEL_CRITICAL, ## __VA_ARGS__), Logme::Instance, Logme::Level::LEVEL_CRITICAL, ## __VA_ARGS__)

/// <summary>
/// Writes CRITICAL log message when condition is true (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="condition">Expression checked before logging.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeC_If(condition, ...) \
  Logme_If(condition, Logme::Instance, Logme::Level::LEVEL_CRITICAL, ## __VA_ARGS__)

/// <summary>
/// Writes DEBUG log message using global CH/SUBSID (printf-style when called with a format string) or returns a stream (C++ style of output).
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeDg(...) \
  Logme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, ## __VA_ARGS__)

/// <summary>
/// Writes DEBUG log message using global CH/SUBSID when condition is true (printf-style when called with a format string) or returns a stream (C++ style of output).
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="condition">Expression checked before logging.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeDg_If(condition, ...) \
  Logme_Ifg(condition, Logme::Instance, Logme::Level::LEVEL_DEBUG, ## __VA_ARGS__)

/// <summary>
/// Writes INFO log message using global CH/SUBSID (printf-style when called with a format string) or returns a stream (C++ style of output).
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeIg(...) \
  Logme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, ## __VA_ARGS__)

/// <summary>
/// Writes INFO log message using global CH/SUBSID when condition is true (printf-style when called with a format string) or returns a stream (C++ style of output).
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="condition">Expression checked before logging.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeI_Ifg(condition, ...) \
  Logme_Ifg(condition, Logme::Instance, Logme::Level::LEVEL_INFO, ## __VA_ARGS__)

/// <summary>
/// Writes WARN log message using global CH/SUBSID (printf-style when called with a format string) or returns a stream (C++ style of output).
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeWg(...) \
  Logme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, ## __VA_ARGS__)

/// <summary>
/// Writes WARN log message using global CH/SUBSID when condition is true (printf-style when called with a format string) or returns a stream (C++ style of output).
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="condition">Expression checked before logging.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeW_Ifg(condition, ...) \
  Logme_Ifg(condition, Logme::Instance, Logme::Level::LEVEL_WARN, ## __VA_ARGS__)

/// <summary>
/// Writes ERROR log message using global CH/SUBSID (printf-style when called with a format string) or returns a stream (C++ style of output).
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeEg(...) \
  Logme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, ## __VA_ARGS__)

/// <summary>
/// Writes ERROR log message using global CH/SUBSID when condition is true (printf-style when called with a format string) or returns a stream (C++ style of output).
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="condition">Expression checked before logging.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeE_Ifg(condition, ...) \
  Logme_Ifg(condition, Logme::Instance, Logme::Level::LEVEL_ERROR, ## __VA_ARGS__)

/// <summary>
/// Writes CRITICAL log message using global CH/SUBSID (printf-style when called with a format string) or returns a stream (C++ style of output).
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeCg(...) \
  Logme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, ## __VA_ARGS__)

/// <summary>
/// Writes CRITICAL log message using global CH/SUBSID when condition is true (printf-style when called with a format string) or returns a stream (C++ style of output).
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="condition">Expression checked before logging.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeC_Ifg(condition, ...) \
  Logme_Ifg(condition, Logme::Instance, Logme::Level::LEVEL_CRITICAL, ## __VA_ARGS__)


#define LogmeD_Do0(ch, ...) \
  do \
  { \
    auto&& _logme_ch_ = (ch); \
    if (Logme::Instance->Condition()) \
    { \
      const auto& _logme_resolved_ch_ = Logme::Detail::ResolveDoChannel(Logme::Instance.get(), _logme_ch_); \
      if (Logme::Detail::WouldLog(Logme::Instance.get(), Logme::Level::LEVEL_DEBUG, _logme_resolved_ch_)) \
      { \
        static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); \
        Logme::Detail::Dispatch(Logme::Instance, LOGME_JOIN(_logme_ctx_, __LINE__), Logme::Level::LEVEL_DEBUG, &CH, &SUBSID, __FUNCTION__, __FILE__, __LINE__, _logme_ch_, ## __VA_ARGS__); \
      } \
    } \
  } while (0)
#define LogmeI_Do0(ch, ...) \
  do \
  { \
    auto&& _logme_ch_ = (ch); \
    if (Logme::Instance->Condition()) \
    { \
      const auto& _logme_resolved_ch_ = Logme::Detail::ResolveDoChannel(Logme::Instance.get(), _logme_ch_); \
      if (Logme::Detail::WouldLog(Logme::Instance.get(), Logme::Level::LEVEL_INFO, _logme_resolved_ch_)) \
      { \
        static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); \
        Logme::Detail::Dispatch(Logme::Instance, LOGME_JOIN(_logme_ctx_, __LINE__), Logme::Level::LEVEL_INFO, &CH, &SUBSID, __FUNCTION__, __FILE__, __LINE__, _logme_ch_, ## __VA_ARGS__); \
      } \
    } \
  } while (0)
#define LogmeW_Do0(ch, ...) \
  do \
  { \
    auto&& _logme_ch_ = (ch); \
    if (Logme::Instance->Condition()) \
    { \
      const auto& _logme_resolved_ch_ = Logme::Detail::ResolveDoChannel(Logme::Instance.get(), _logme_ch_); \
      if (Logme::Detail::WouldLog(Logme::Instance.get(), Logme::Level::LEVEL_WARN, _logme_resolved_ch_)) \
      { \
        static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); \
        Logme::Detail::Dispatch(Logme::Instance, LOGME_JOIN(_logme_ctx_, __LINE__), Logme::Level::LEVEL_WARN, &CH, &SUBSID, __FUNCTION__, __FILE__, __LINE__, _logme_ch_, ## __VA_ARGS__); \
      } \
    } \
  } while (0)
#define LogmeE_Do0(ch, ...) \
  do \
  { \
    auto&& _logme_ch_ = (ch); \
    if (Logme::Instance->Condition()) \
    { \
      const auto& _logme_resolved_ch_ = Logme::Detail::ResolveDoChannel(Logme::Instance.get(), _logme_ch_); \
      if (Logme::Detail::WouldLog(Logme::Instance.get(), Logme::Level::LEVEL_ERROR, _logme_resolved_ch_)) \
      { \
        static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); \
        Logme::Detail::Dispatch(Logme::Instance, LOGME_JOIN(_logme_ctx_, __LINE__), Logme::Level::LEVEL_ERROR, &CH, &SUBSID, __FUNCTION__, __FILE__, __LINE__, _logme_ch_, ## __VA_ARGS__); \
      } \
    } \
  } while (0)
#define LogmeC_Do0(ch, ...) \
  do \
  { \
    auto&& _logme_ch_ = (ch); \
    if (Logme::Instance->Condition()) \
    { \
      const auto& _logme_resolved_ch_ = Logme::Detail::ResolveDoChannel(Logme::Instance.get(), _logme_ch_); \
      if (Logme::Detail::WouldLog(Logme::Instance.get(), Logme::Level::LEVEL_CRITICAL, _logme_resolved_ch_)) \
      { \
        static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); \
        Logme::Detail::Dispatch(Logme::Instance, LOGME_JOIN(_logme_ctx_, __LINE__), Logme::Level::LEVEL_CRITICAL, &CH, &SUBSID, __FUNCTION__, __FILE__, __LINE__, _logme_ch_, ## __VA_ARGS__); \
      } \
    } \
  } while (0)
/// <summary>
/// Executes code and writes DEBUG message using printf-style formatting only when the selected channel would log this level.
/// </summary>
/// <param name="ch">Channel id or channel pointer.</param>
/// <param name="code">Code executed before writing the message.</param>
/// <param name="...">Format and format arguments.</param>
#define LogmeD_Do(ch, code, ...) do { auto&& _logme_ch_ = (ch); if (Logme::Instance->Condition()) { const auto& _logme_resolved_ch_ = Logme::Detail::ResolveDoChannel(Logme::Instance.get(), _logme_ch_); if (Logme::Detail::WouldLog(Logme::Instance.get(), Logme::Level::LEVEL_DEBUG, _logme_resolved_ch_)) { code; static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); Logme::Detail::Dispatch(Logme::Instance, LOGME_JOIN(_logme_ctx_, __LINE__), Logme::Level::LEVEL_DEBUG, &CH, &SUBSID, __FUNCTION__, __FILE__, __LINE__, _logme_ch_, ## __VA_ARGS__); } } } while (0)
/// <summary>
/// Executes code and writes INFO message using printf-style formatting only when the selected channel would log this level.
/// </summary>
/// <param name="ch">Channel id or channel pointer.</param>
/// <param name="code">Code executed before writing the message.</param>
/// <param name="...">Format and format arguments.</param>
#define LogmeI_Do(ch, code, ...) do { auto&& _logme_ch_ = (ch); if (Logme::Instance->Condition()) { const auto& _logme_resolved_ch_ = Logme::Detail::ResolveDoChannel(Logme::Instance.get(), _logme_ch_); if (Logme::Detail::WouldLog(Logme::Instance.get(), Logme::Level::LEVEL_INFO, _logme_resolved_ch_)) { code; static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); Logme::Detail::Dispatch(Logme::Instance, LOGME_JOIN(_logme_ctx_, __LINE__), Logme::Level::LEVEL_INFO, &CH, &SUBSID, __FUNCTION__, __FILE__, __LINE__, _logme_ch_, ## __VA_ARGS__); } } } while (0)
/// <summary>
/// Executes code and writes WARN message using printf-style formatting only when the selected channel would log this level.
/// </summary>
/// <param name="ch">Channel id or channel pointer.</param>
/// <param name="code">Code executed before writing the message.</param>
/// <param name="...">Format and format arguments.</param>
#define LogmeW_Do(ch, code, ...) do { auto&& _logme_ch_ = (ch); if (Logme::Instance->Condition()) { const auto& _logme_resolved_ch_ = Logme::Detail::ResolveDoChannel(Logme::Instance.get(), _logme_ch_); if (Logme::Detail::WouldLog(Logme::Instance.get(), Logme::Level::LEVEL_WARN, _logme_resolved_ch_)) { code; static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); Logme::Detail::Dispatch(Logme::Instance, LOGME_JOIN(_logme_ctx_, __LINE__), Logme::Level::LEVEL_WARN, &CH, &SUBSID, __FUNCTION__, __FILE__, __LINE__, _logme_ch_, ## __VA_ARGS__); } } } while (0)
/// <summary>
/// Executes code and writes ERROR message using printf-style formatting only when the selected channel would log this level.
/// </summary>
/// <param name="ch">Channel id or channel pointer.</param>
/// <param name="code">Code executed before writing the message.</param>
/// <param name="...">Format and format arguments.</param>
#define LogmeE_Do(ch, code, ...) do { auto&& _logme_ch_ = (ch); if (Logme::Instance->Condition()) { const auto& _logme_resolved_ch_ = Logme::Detail::ResolveDoChannel(Logme::Instance.get(), _logme_ch_); if (Logme::Detail::WouldLog(Logme::Instance.get(), Logme::Level::LEVEL_ERROR, _logme_resolved_ch_)) { code; static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); Logme::Detail::Dispatch(Logme::Instance, LOGME_JOIN(_logme_ctx_, __LINE__), Logme::Level::LEVEL_ERROR, &CH, &SUBSID, __FUNCTION__, __FILE__, __LINE__, _logme_ch_, ## __VA_ARGS__); } } } while (0)
/// <summary>
/// Executes code and writes CRITICAL message using printf-style formatting only when the selected channel would log this level.
/// </summary>
/// <param name="ch">Channel id or channel pointer.</param>
/// <param name="code">Code executed before writing the message.</param>
/// <param name="...">Format and format arguments.</param>
#define LogmeC_Do(ch, code, ...) do { auto&& _logme_ch_ = (ch); if (Logme::Instance->Condition()) { const auto& _logme_resolved_ch_ = Logme::Detail::ResolveDoChannel(Logme::Instance.get(), _logme_ch_); if (Logme::Detail::WouldLog(Logme::Instance.get(), Logme::Level::LEVEL_CRITICAL, _logme_resolved_ch_)) { code; static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); Logme::Detail::Dispatch(Logme::Instance, LOGME_JOIN(_logme_ctx_, __LINE__), Logme::Level::LEVEL_CRITICAL, &CH, &SUBSID, __FUNCTION__, __FILE__, __LINE__, _logme_ch_, ## __VA_ARGS__); } } } while (0)
#define LogmeD_Do1(ch, code, ...) LogmeD_Do(ch, code, ## __VA_ARGS__)
#define LogmeI_Do1(ch, code, ...) LogmeI_Do(ch, code, ## __VA_ARGS__)
#define LogmeW_Do1(ch, code, ...) LogmeW_Do(ch, code, ## __VA_ARGS__)
#define LogmeE_Do1(ch, code, ...) LogmeE_Do(ch, code, ## __VA_ARGS__)
#define LogmeC_Do1(ch, code, ...) LogmeC_Do(ch, code, ## __VA_ARGS__)

#if _LOGME_ACTIVE
  #define _LOGME_ONCE_OVR() ([]() -> Logme::Override& { static Logme::Override ovr(1); return ovr; }())
  #define _LOGME_RATE_OVR(ms) ([]() -> Logme::Override& { static Logme::Override ovr(-1, ms); return ovr; }())
#else
  #define _LOGME_ONCE_OVR()
  #define _LOGME_RATE_OVR(ms)
#endif

// Log once (per call site)

#ifdef _MSC_VER
/// <summary>
/// Writes DEBUG log message once per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeD_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, _LOGME_ONCE_OVR(), ## __VA_ARGS__)
#else
/// <summary>
/// Writes DEBUG log message once per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeD_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, _LOGME_ONCE_OVR() _LOGME_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
/// <summary>
/// Writes INFO log message once per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeI_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, _LOGME_ONCE_OVR(), ## __VA_ARGS__)
#else
/// <summary>
/// Writes INFO log message once per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeI_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, _LOGME_ONCE_OVR() _LOGME_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
/// <summary>
/// Writes WARN log message once per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeW_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, _LOGME_ONCE_OVR(), ## __VA_ARGS__)
#else
/// <summary>
/// Writes WARN log message once per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeW_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, _LOGME_ONCE_OVR() _LOGME_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
/// <summary>
/// Writes ERROR log message once per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeE_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, _LOGME_ONCE_OVR(), ## __VA_ARGS__)
#else
/// <summary>
/// Writes ERROR log message once per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeE_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, _LOGME_ONCE_OVR() _LOGME_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
/// <summary>
/// Writes CRITICAL log message once per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeC_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, _LOGME_ONCE_OVR(), ## __VA_ARGS__)
#else
/// <summary>
/// Writes CRITICAL log message once per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeC_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, _LOGME_ONCE_OVR() _LOGME_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

// Log with interval (rate-limited)

#ifdef _MSC_VER
/// <summary>
/// Writes DEBUG log message at most once per interval per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeD_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, _LOGME_RATE_OVR(ms), ## __VA_ARGS__)
#else
/// <summary>
/// Writes DEBUG log message at most once per interval per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeD_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, _LOGME_RATE_OVR(ms) _LOGME_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
/// <summary>
/// Writes INFO log message at most once per interval per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeI_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, _LOGME_RATE_OVR(ms), ## __VA_ARGS__)
#else
/// <summary>
/// Writes INFO log message at most once per interval per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeI_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, _LOGME_RATE_OVR(ms) _LOGME_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
/// <summary>
/// Writes WARN log message at most once per interval per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeW_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, _LOGME_RATE_OVR(ms), ## __VA_ARGS__)
#else
/// <summary>
/// Writes WARN log message at most once per interval per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeW_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, _LOGME_RATE_OVR(ms) _LOGME_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
/// <summary>
/// Writes ERROR log message at most once per interval per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeE_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, _LOGME_RATE_OVR(ms), ## __VA_ARGS__)
#else
/// <summary>
/// Writes ERROR log message at most once per interval per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeE_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, _LOGME_RATE_OVR(ms) _LOGME_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
/// <summary>
/// Writes CRITICAL log message at most once per interval per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeC_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, _LOGME_RATE_OVR(ms), ## __VA_ARGS__)
#else
/// <summary>
/// Writes CRITICAL log message at most once per interval per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
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

// Public std::format logging macros accept the same optional argument set:
//   [channel/id or ChannelPtr], [Override], [SID], format, args...
// Override may appear before or after channel where supported by Logger overloads.

/// <summary>
/// Writes DEBUG message using std::format-style formatting.
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format and format arguments.</param>
#define fLogmeD(...) \
  fLogme_If(Logme::Instance->Condition() && LOGME_WOULD_LOG_FIRST(Logme::Instance.get(), Logme::Level::LEVEL_DEBUG, ## __VA_ARGS__), Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), ## __VA_ARGS__)

/// <summary>
/// Writes DEBUG message using std::format-style formatting when condition is true.
/// </summary>
/// <param name="condition">Expression checked before logging.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format and format arguments.</param>
#define fLogmeD_If(condition, ...) \
  fLogme_If(condition, Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), ## __VA_ARGS__)

/// <summary>
/// Writes INFO message using std::format-style formatting.
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format and format arguments.</param>
#define fLogmeI(...) \
  fLogme_If(Logme::Instance->Condition() && LOGME_WOULD_LOG_FIRST(Logme::Instance.get(), Logme::Level::LEVEL_INFO, ## __VA_ARGS__), Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), ## __VA_ARGS__)

/// <summary>
/// Writes INFO message using std::format-style formatting when condition is true.
/// </summary>
/// <param name="condition">Expression checked before logging.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format and format arguments.</param>
#define fLogmeI_If(condition, ...) \
  fLogme_If(condition, Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), ## __VA_ARGS__)

/// <summary>
/// Writes WARN message using std::format-style formatting.
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format and format arguments.</param>
#define fLogmeW(...) \
  fLogme_If(Logme::Instance->Condition() && LOGME_WOULD_LOG_FIRST(Logme::Instance.get(), Logme::Level::LEVEL_WARN, ## __VA_ARGS__), Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), ## __VA_ARGS__)

/// <summary>
/// Writes WARN message using std::format-style formatting when condition is true.
/// </summary>
/// <param name="condition">Expression checked before logging.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format and format arguments.</param>
#define fLogmeW_If(condition, ...) \
  fLogme_If(condition, Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), ## __VA_ARGS__)

/// <summary>
/// Writes ERROR message using std::format-style formatting.
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format and format arguments.</param>
#define fLogmeE(...) \
  fLogme_If(Logme::Instance->Condition() && LOGME_WOULD_LOG_FIRST(Logme::Instance.get(), Logme::Level::LEVEL_ERROR, ## __VA_ARGS__), Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), ## __VA_ARGS__)

/// <summary>
/// Writes ERROR message using std::format-style formatting when condition is true.
/// </summary>
/// <param name="condition">Expression checked before logging.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format and format arguments.</param>
#define fLogmeE_If(condition, ...) \
  fLogme_If(condition, Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), ## __VA_ARGS__)

/// <summary>
/// Writes CRITICAL message using std::format-style formatting.
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format and format arguments.</param>
#define fLogmeC(...) \
  fLogme_If(Logme::Instance->Condition() && LOGME_WOULD_LOG_FIRST(Logme::Instance.get(), Logme::Level::LEVEL_CRITICAL, ## __VA_ARGS__), Logme::Instance, Logme::Level::LEVEL_CRITICAL, Logme::GetStdFormat(), ## __VA_ARGS__)

/// <summary>
/// Writes CRITICAL message using std::format-style formatting when condition is true.
/// </summary>
/// <param name="condition">Expression checked before logging.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format and format arguments.</param>
#define fLogmeC_If(condition, ...) \
  fLogme_If(condition, Logme::Instance, Logme::Level::LEVEL_CRITICAL, Logme::GetStdFormat(), ## __VA_ARGS__)

/// <summary>
/// Writes DEBUG message using std::format-style formatting and global CH/SUBSID.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format and format arguments.</param>
#define fLogmeDg(...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), ## __VA_ARGS__)

/// <summary>
/// Writes DEBUG message using std::format-style formatting and global CH/SUBSID when condition is true.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="condition">Expression checked before logging.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format and format arguments.</param>
#define fLogmeDg_If(condition, ...) \
  fLogme_Ifg(condition, Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), ## __VA_ARGS__)

/// <summary>
/// Writes INFO message using std::format-style formatting and global CH/SUBSID.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format and format arguments.</param>
#define fLogmeIg(...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), ## __VA_ARGS__)

/// <summary>
/// Writes INFO message using std::format-style formatting and global CH/SUBSID when condition is true.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="condition">Expression checked before logging.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format and format arguments.</param>
#define fLogmeI_Ifg(condition, ...) \
  fLogme_Ifg(condition, Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), ## __VA_ARGS__)

/// <summary>
/// Writes WARN message using std::format-style formatting and global CH/SUBSID.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format and format arguments.</param>
#define fLogmeWg(...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), ## __VA_ARGS__)

/// <summary>
/// Writes WARN message using std::format-style formatting and global CH/SUBSID when condition is true.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="condition">Expression checked before logging.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format and format arguments.</param>
#define fLogmeW_Ifg(condition, ...) \
  fLogme_Ifg(condition, Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), ## __VA_ARGS__)

/// <summary>
/// Writes ERROR message using std::format-style formatting and global CH/SUBSID.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format and format arguments.</param>
#define fLogmeEg(...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), ## __VA_ARGS__)

/// <summary>
/// Writes ERROR message using std::format-style formatting and global CH/SUBSID when condition is true.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="condition">Expression checked before logging.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format and format arguments.</param>
#define fLogmeE_Ifg(condition, ...) \
  fLogme_Ifg(condition, Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), ## __VA_ARGS__)

/// <summary>
/// Writes DEBUG message using std::format-style formatting once per call site.
/// </summary>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeD_Once(...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), _LOGME_ONCE_OVR(), ## __VA_ARGS__)

/// <summary>
/// Writes INFO message using std::format-style formatting once per call site.
/// </summary>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeI_Once(...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), _LOGME_ONCE_OVR(), ## __VA_ARGS__)

/// <summary>
/// Writes WARN message using std::format-style formatting once per call site.
/// </summary>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeW_Once(...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), _LOGME_ONCE_OVR(), ## __VA_ARGS__)

/// <summary>
/// Writes ERROR message using std::format-style formatting once per call site.
/// </summary>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeE_Once(...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), _LOGME_ONCE_OVR(), ## __VA_ARGS__)

/// <summary>
/// Writes CRITICAL message using std::format-style formatting once per call site.
/// </summary>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeC_Once(...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, Logme::GetStdFormat(), _LOGME_ONCE_OVR(), ## __VA_ARGS__)

/// <summary>
/// Writes DEBUG message using std::format-style formatting and global CH/SUBSID once per call site.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeDg_Once(...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), _LOGME_ONCE_OVR(), ## __VA_ARGS__)

/// <summary>
/// Writes INFO message using std::format-style formatting and global CH/SUBSID once per call site.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeIg_Once(...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), _LOGME_ONCE_OVR(), ## __VA_ARGS__)

/// <summary>
/// Writes WARN message using std::format-style formatting and global CH/SUBSID once per call site.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeWg_Once(...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), _LOGME_ONCE_OVR(), ## __VA_ARGS__)

/// <summary>
/// Writes ERROR message using std::format-style formatting and global CH/SUBSID once per call site.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeEg_Once(...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), _LOGME_ONCE_OVR(), ## __VA_ARGS__)

/// <summary>
/// Writes CRITICAL message using std::format-style formatting and global CH/SUBSID once per call site.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeCg_Once(...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, Logme::GetStdFormat(), _LOGME_ONCE_OVR(), ## __VA_ARGS__)

/// <summary>
/// Writes DEBUG message using std::format-style formatting at most once per interval per call site.
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeD_Every(ms, ...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), _LOGME_RATE_OVR(ms), ## __VA_ARGS__)

/// <summary>
/// Writes INFO message using std::format-style formatting at most once per interval per call site.
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeI_Every(ms, ...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), _LOGME_RATE_OVR(ms), ## __VA_ARGS__)

/// <summary>
/// Writes WARN message using std::format-style formatting at most once per interval per call site.
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeW_Every(ms, ...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), _LOGME_RATE_OVR(ms), ## __VA_ARGS__)

/// <summary>
/// Writes ERROR message using std::format-style formatting at most once per interval per call site.
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeE_Every(ms, ...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), _LOGME_RATE_OVR(ms), ## __VA_ARGS__)

/// <summary>
/// Writes CRITICAL message using std::format-style formatting at most once per interval per call site.
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeC_Every(ms, ...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, Logme::GetStdFormat(), _LOGME_RATE_OVR(ms), ## __VA_ARGS__)

/// <summary>
/// Writes DEBUG message using std::format-style formatting and global CH/SUBSID at most once per interval per call site.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeDg_Every(ms, ...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), _LOGME_RATE_OVR(ms), ## __VA_ARGS__)

/// <summary>
/// Writes INFO message using std::format-style formatting and global CH/SUBSID at most once per interval per call site.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeIg_Every(ms, ...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), _LOGME_RATE_OVR(ms), ## __VA_ARGS__)

/// <summary>
/// Writes WARN message using std::format-style formatting and global CH/SUBSID at most once per interval per call site.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeWg_Every(ms, ...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), _LOGME_RATE_OVR(ms), ## __VA_ARGS__)

/// <summary>
/// Writes ERROR message using std::format-style formatting and global CH/SUBSID at most once per interval per call site.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeEg_Every(ms, ...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), _LOGME_RATE_OVR(ms), ## __VA_ARGS__)

/// <summary>
/// Writes CRITICAL message using std::format-style formatting and global CH/SUBSID at most once per interval per call site.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeCg_Every(ms, ...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, Logme::GetStdFormat(), _LOGME_RATE_OVR(ms), ## __VA_ARGS__)


#define fLogmeD_Do0(ch, ...) do { auto&& _logme_ch_ = (ch); if (Logme::Instance->Condition()) { const auto& _logme_resolved_ch_ = Logme::Detail::ResolveDoChannel(Logme::Instance.get(), _logme_ch_); if (Logme::Detail::WouldLog(Logme::Instance.get(), Logme::Level::LEVEL_DEBUG, _logme_resolved_ch_)) { LOGME_PRAGMA_PUSH LOGME_PRAGMA_IGNORE_VARARGS static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); Logme::Detail::DispatchStdFormat(Logme::Instance, LOGME_JOIN(_logme_ctx_, __LINE__), Logme::Level::LEVEL_DEBUG, &CH, &SUBSID, __FUNCTION__, __FILE__, __LINE__, Logme::GetStdFormat(), _logme_ch_, ## __VA_ARGS__); LOGME_PRAGMA_POP } } } while (0)
#define fLogmeI_Do0(ch, ...) do { auto&& _logme_ch_ = (ch); if (Logme::Instance->Condition()) { const auto& _logme_resolved_ch_ = Logme::Detail::ResolveDoChannel(Logme::Instance.get(), _logme_ch_); if (Logme::Detail::WouldLog(Logme::Instance.get(), Logme::Level::LEVEL_INFO, _logme_resolved_ch_)) { LOGME_PRAGMA_PUSH LOGME_PRAGMA_IGNORE_VARARGS static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); Logme::Detail::DispatchStdFormat(Logme::Instance, LOGME_JOIN(_logme_ctx_, __LINE__), Logme::Level::LEVEL_INFO, &CH, &SUBSID, __FUNCTION__, __FILE__, __LINE__, Logme::GetStdFormat(), _logme_ch_, ## __VA_ARGS__); LOGME_PRAGMA_POP } } } while (0)
#define fLogmeW_Do0(ch, ...) do { auto&& _logme_ch_ = (ch); if (Logme::Instance->Condition()) { const auto& _logme_resolved_ch_ = Logme::Detail::ResolveDoChannel(Logme::Instance.get(), _logme_ch_); if (Logme::Detail::WouldLog(Logme::Instance.get(), Logme::Level::LEVEL_WARN, _logme_resolved_ch_)) { LOGME_PRAGMA_PUSH LOGME_PRAGMA_IGNORE_VARARGS static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); Logme::Detail::DispatchStdFormat(Logme::Instance, LOGME_JOIN(_logme_ctx_, __LINE__), Logme::Level::LEVEL_WARN, &CH, &SUBSID, __FUNCTION__, __FILE__, __LINE__, Logme::GetStdFormat(), _logme_ch_, ## __VA_ARGS__); LOGME_PRAGMA_POP } } } while (0)
#define fLogmeE_Do0(ch, ...) do { auto&& _logme_ch_ = (ch); if (Logme::Instance->Condition()) { const auto& _logme_resolved_ch_ = Logme::Detail::ResolveDoChannel(Logme::Instance.get(), _logme_ch_); if (Logme::Detail::WouldLog(Logme::Instance.get(), Logme::Level::LEVEL_ERROR, _logme_resolved_ch_)) { LOGME_PRAGMA_PUSH LOGME_PRAGMA_IGNORE_VARARGS static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); Logme::Detail::DispatchStdFormat(Logme::Instance, LOGME_JOIN(_logme_ctx_, __LINE__), Logme::Level::LEVEL_ERROR, &CH, &SUBSID, __FUNCTION__, __FILE__, __LINE__, Logme::GetStdFormat(), _logme_ch_, ## __VA_ARGS__); LOGME_PRAGMA_POP } } } while (0)
#define fLogmeC_Do0(ch, ...) do { auto&& _logme_ch_ = (ch); if (Logme::Instance->Condition()) { const auto& _logme_resolved_ch_ = Logme::Detail::ResolveDoChannel(Logme::Instance.get(), _logme_ch_); if (Logme::Detail::WouldLog(Logme::Instance.get(), Logme::Level::LEVEL_CRITICAL, _logme_resolved_ch_)) { LOGME_PRAGMA_PUSH LOGME_PRAGMA_IGNORE_VARARGS static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); Logme::Detail::DispatchStdFormat(Logme::Instance, LOGME_JOIN(_logme_ctx_, __LINE__), Logme::Level::LEVEL_CRITICAL, &CH, &SUBSID, __FUNCTION__, __FILE__, __LINE__, Logme::GetStdFormat(), _logme_ch_, ## __VA_ARGS__); LOGME_PRAGMA_POP } } } while (0)
/// <summary>
/// Executes code and writes DEBUG message using std::format-style formatting only when the selected channel would log this level.
/// </summary>
/// <param name="ch">Channel id or channel pointer.</param>
/// <param name="code">Code executed before writing the message.</param>
/// <param name="...">Format and format arguments.</param>
#define fLogmeD_Do(ch, code, ...) do { auto&& _logme_ch_ = (ch); if (Logme::Instance->Condition()) { const auto& _logme_resolved_ch_ = Logme::Detail::ResolveDoChannel(Logme::Instance.get(), _logme_ch_); if (Logme::Detail::WouldLog(Logme::Instance.get(), Logme::Level::LEVEL_DEBUG, _logme_resolved_ch_)) { code; LOGME_PRAGMA_PUSH LOGME_PRAGMA_IGNORE_VARARGS static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); Logme::Detail::DispatchStdFormat(Logme::Instance, LOGME_JOIN(_logme_ctx_, __LINE__), Logme::Level::LEVEL_DEBUG, &CH, &SUBSID, __FUNCTION__, __FILE__, __LINE__, Logme::GetStdFormat(), _logme_ch_, ## __VA_ARGS__); LOGME_PRAGMA_POP } } } while (0)
/// <summary>
/// Executes code and writes INFO message using std::format-style formatting only when the selected channel would log this level.
/// </summary>
/// <param name="ch">Channel id or channel pointer.</param>
/// <param name="code">Code executed before writing the message.</param>
/// <param name="...">Format and format arguments.</param>
#define fLogmeI_Do(ch, code, ...) do { auto&& _logme_ch_ = (ch); if (Logme::Instance->Condition()) { const auto& _logme_resolved_ch_ = Logme::Detail::ResolveDoChannel(Logme::Instance.get(), _logme_ch_); if (Logme::Detail::WouldLog(Logme::Instance.get(), Logme::Level::LEVEL_INFO, _logme_resolved_ch_)) { code; LOGME_PRAGMA_PUSH LOGME_PRAGMA_IGNORE_VARARGS static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); Logme::Detail::DispatchStdFormat(Logme::Instance, LOGME_JOIN(_logme_ctx_, __LINE__), Logme::Level::LEVEL_INFO, &CH, &SUBSID, __FUNCTION__, __FILE__, __LINE__, Logme::GetStdFormat(), _logme_ch_, ## __VA_ARGS__); LOGME_PRAGMA_POP } } } while (0)
/// <summary>
/// Executes code and writes WARN message using std::format-style formatting only when the selected channel would log this level.
/// </summary>
/// <param name="ch">Channel id or channel pointer.</param>
/// <param name="code">Code executed before writing the message.</param>
/// <param name="...">Format and format arguments.</param>
#define fLogmeW_Do(ch, code, ...) do { auto&& _logme_ch_ = (ch); if (Logme::Instance->Condition()) { const auto& _logme_resolved_ch_ = Logme::Detail::ResolveDoChannel(Logme::Instance.get(), _logme_ch_); if (Logme::Detail::WouldLog(Logme::Instance.get(), Logme::Level::LEVEL_WARN, _logme_resolved_ch_)) { code; LOGME_PRAGMA_PUSH LOGME_PRAGMA_IGNORE_VARARGS static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); Logme::Detail::DispatchStdFormat(Logme::Instance, LOGME_JOIN(_logme_ctx_, __LINE__), Logme::Level::LEVEL_WARN, &CH, &SUBSID, __FUNCTION__, __FILE__, __LINE__, Logme::GetStdFormat(), _logme_ch_, ## __VA_ARGS__); LOGME_PRAGMA_POP } } } while (0)
/// <summary>
/// Executes code and writes ERROR message using std::format-style formatting only when the selected channel would log this level.
/// </summary>
/// <param name="ch">Channel id or channel pointer.</param>
/// <param name="code">Code executed before writing the message.</param>
/// <param name="...">Format and format arguments.</param>
#define fLogmeE_Do(ch, code, ...) do { auto&& _logme_ch_ = (ch); if (Logme::Instance->Condition()) { const auto& _logme_resolved_ch_ = Logme::Detail::ResolveDoChannel(Logme::Instance.get(), _logme_ch_); if (Logme::Detail::WouldLog(Logme::Instance.get(), Logme::Level::LEVEL_ERROR, _logme_resolved_ch_)) { code; LOGME_PRAGMA_PUSH LOGME_PRAGMA_IGNORE_VARARGS static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); Logme::Detail::DispatchStdFormat(Logme::Instance, LOGME_JOIN(_logme_ctx_, __LINE__), Logme::Level::LEVEL_ERROR, &CH, &SUBSID, __FUNCTION__, __FILE__, __LINE__, Logme::GetStdFormat(), _logme_ch_, ## __VA_ARGS__); LOGME_PRAGMA_POP } } } while (0)
/// <summary>
/// Executes code and writes CRITICAL message using std::format-style formatting only when the selected channel would log this level.
/// </summary>
/// <param name="ch">Channel id or channel pointer.</param>
/// <param name="code">Code executed before writing the message.</param>
/// <param name="...">Format and format arguments.</param>
#define fLogmeC_Do(ch, code, ...) do { auto&& _logme_ch_ = (ch); if (Logme::Instance->Condition()) { const auto& _logme_resolved_ch_ = Logme::Detail::ResolveDoChannel(Logme::Instance.get(), _logme_ch_); if (Logme::Detail::WouldLog(Logme::Instance.get(), Logme::Level::LEVEL_CRITICAL, _logme_resolved_ch_)) { code; LOGME_PRAGMA_PUSH LOGME_PRAGMA_IGNORE_VARARGS static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); Logme::Detail::DispatchStdFormat(Logme::Instance, LOGME_JOIN(_logme_ctx_, __LINE__), Logme::Level::LEVEL_CRITICAL, &CH, &SUBSID, __FUNCTION__, __FILE__, __LINE__, Logme::GetStdFormat(), _logme_ch_, ## __VA_ARGS__); LOGME_PRAGMA_POP } } } while (0)
#define fLogmeD_Do1(ch, code, ...) fLogmeD_Do(ch, code, ## __VA_ARGS__)
#define fLogmeI_Do1(ch, code, ...) fLogmeI_Do(ch, code, ## __VA_ARGS__)
#define fLogmeW_Do1(ch, code, ...) fLogmeW_Do(ch, code, ## __VA_ARGS__)
#define fLogmeE_Do1(ch, code, ...) fLogmeE_Do(ch, code, ## __VA_ARGS__)
#define fLogmeC_Do1(ch, code, ...) fLogmeC_Do(ch, code, ## __VA_ARGS__)

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

/// <summary>Sets the current thread channel for the current scope.</summary>
/// <param name="ch">Channel id or channel pointer.</param>
#define LogmeThreadChannel(ch) \
  Logme::ThreadChannel _logme_thread_channel(Logme::Instance, ch)

// Thread name

/// <summary>Sets the current thread name and optionally logs it to the channel.</summary>
/// <param name="pch">Channel pointer used for the thread-name record.</param>
/// <param name="...">Format and format arguments for the thread name.</param>
#define LogmeThreadName(pch, ...) \
  Logme::ThreadName _logme_thread_name(pch, __VA_ARGS__)

/// <summary>Returns true when the current thread has an explicitly assigned channel.</summary>
#define LogmeThreadChannelDefined() \
  Logme::Instance->IsChannelDefinedForCurrentThread()

// Thread override

/// <summary>Sets the current thread output override for the current scope.</summary>
/// <param name="ovr">Override object used by log records from this scope.</param>
#define LogmeThreadOverride(ovr) \
  Logme::ThreadOverride _logme_thread_override(Logme::Instance, ovr)
