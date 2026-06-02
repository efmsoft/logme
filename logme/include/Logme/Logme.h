#pragma once

#ifndef __cplusplus
#include <Logme/LogmeC.h>
#else

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
#include <Logme/ThreadSubsystem.h>
#include <Logme/TracePoint.h>
#include <utility>

// Performs simple encoding conversion. If you need reliable, high-quality conversion in all cases, 
// use https://github.com/efmsoft/utf8 and the functions utf8::AnsiToUtf8 / WstringToUtf8

#ifndef LS
#define LS(str) Logme::ToStdString(str)
#endif

#ifndef LWS
#define LWS(str) Logme::ToStdWString(str)
#endif

#include <Logme/Detail/Dispatch.h>
#include <Logme/Detail/Precheck.h>

// C/C++ - style logging

#if LOGME_ACTIVE
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

#ifdef _MSC_VER
  #define Logme_CollapseAt(level, limit, ...) \
    if (static Logme::CollapseContextCache LOGME_JOIN(_logme_ctx_, __LINE__)(limit); Logme::Instance->Condition() && LOGME_WOULD_LOG_FIRST(Logme::Instance.get(), level, ## __VA_ARGS__)) \
      Logme::Detail::DispatchCollapse( \
        Logme::Instance \
        , LOGME_JOIN(_logme_ctx_, __LINE__) \
        , level \
        , &CH \
        , &SUBSID \
        , __FUNCTION__ \
        , __FILE__ \
        , __LINE__ \
        , ## __VA_ARGS__ \
      )
  #define Logme_CollapseIgnoreAt(level, ignoreRegex, limit, ...) \
    if (static Logme::CollapseContextCache LOGME_JOIN(_logme_ctx_, __LINE__)(ignoreRegex, limit); Logme::Instance->Condition() && LOGME_WOULD_LOG_FIRST(Logme::Instance.get(), level, ## __VA_ARGS__)) \
      Logme::Detail::DispatchCollapse( \
        Logme::Instance \
        , LOGME_JOIN(_logme_ctx_, __LINE__) \
        , level \
        , &CH \
        , &SUBSID \
        , __FUNCTION__ \
        , __FILE__ \
        , __LINE__ \
        , ## __VA_ARGS__ \
      )
#else
  #define Logme_CollapseAt(level, limit, ...) \
    if (static Logme::CollapseContextCache LOGME_JOIN(_logme_ctx_, __LINE__)(limit); Logme::Instance->Condition() && LOGME_WOULD_LOG_FIRST(Logme::Instance.get(), level, __VA_ARGS__)) \
      Logme::Detail::DispatchCollapse( \
        Logme::Instance \
        , LOGME_JOIN(_logme_ctx_, __LINE__) \
        , level \
        , &CH \
        , &SUBSID \
        , __FUNCTION__ \
        , __FILE__ \
        , __LINE__ \
        , __VA_ARGS__ \
      )
  #define Logme_CollapseIgnoreAt(level, ignoreRegex, limit, ...) \
    if (static Logme::CollapseContextCache LOGME_JOIN(_logme_ctx_, __LINE__)(ignoreRegex, limit); Logme::Instance->Condition() && LOGME_WOULD_LOG_FIRST(Logme::Instance.get(), level, __VA_ARGS__)) \
      Logme::Detail::DispatchCollapse( \
        Logme::Instance \
        , LOGME_JOIN(_logme_ctx_, __LINE__) \
        , level \
        , &CH \
        , &SUBSID \
        , __FUNCTION__ \
        , __FILE__ \
        , __LINE__ \
        , __VA_ARGS__ \
      )
#endif

/// <summary>
/// Writes DEBUG log message and collapses repeated messages using the formatted message text as the repeat key.
/// </summary>
/// <param name="limit">Number of repeated messages to skip before writing the collapsed summary.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format, args, etc.</param>
#define LogmeD_Collapse(limit, ...) \
  Logme_CollapseAt(Logme::Level::LEVEL_DEBUG, limit, ## __VA_ARGS__)

/// <summary>
/// Writes DEBUG log message and collapses repeated messages ignoring substrings matched by a regular expression.
/// </summary>
/// <param name="ignoreRegex">Regular expression used to remove ignored substrings before comparing messages.</param>
/// <param name="limit">Number of repeated messages to skip before writing the collapsed summary.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format, args, etc.</param>
#define LogmeD_CollapseIgnore(ignoreRegex, limit, ...) \
  Logme_CollapseIgnoreAt(Logme::Level::LEVEL_DEBUG, ignoreRegex, limit, ## __VA_ARGS__)

/// <summary>
/// Writes INFO log message and collapses repeated messages using the formatted message text as the repeat key.
/// </summary>
/// <param name="limit">Number of repeated messages to skip before writing the collapsed summary.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format, args, etc.</param>
#define LogmeI_Collapse(limit, ...) \
  Logme_CollapseAt(Logme::Level::LEVEL_INFO, limit, ## __VA_ARGS__)

/// <summary>
/// Writes INFO log message and collapses repeated messages ignoring substrings matched by a regular expression.
/// </summary>
/// <param name="ignoreRegex">Regular expression used to remove ignored substrings before comparing messages.</param>
/// <param name="limit">Number of repeated messages to skip before writing the collapsed summary.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format, args, etc.</param>
#define LogmeI_CollapseIgnore(ignoreRegex, limit, ...) \
  Logme_CollapseIgnoreAt(Logme::Level::LEVEL_INFO, ignoreRegex, limit, ## __VA_ARGS__)

/// <summary>
/// Writes WARN log message and collapses repeated messages using the formatted message text as the repeat key.
/// </summary>
/// <param name="limit">Number of repeated messages to skip before writing the collapsed summary.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format, args, etc.</param>
#define LogmeW_Collapse(limit, ...) \
  Logme_CollapseAt(Logme::Level::LEVEL_WARN, limit, ## __VA_ARGS__)

/// <summary>
/// Writes WARN log message and collapses repeated messages ignoring substrings matched by a regular expression.
/// </summary>
/// <param name="ignoreRegex">Regular expression used to remove ignored substrings before comparing messages.</param>
/// <param name="limit">Number of repeated messages to skip before writing the collapsed summary.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format, args, etc.</param>
#define LogmeW_CollapseIgnore(ignoreRegex, limit, ...) \
  Logme_CollapseIgnoreAt(Logme::Level::LEVEL_WARN, ignoreRegex, limit, ## __VA_ARGS__)

/// <summary>
/// Writes ERROR log message and collapses repeated messages using the formatted message text as the repeat key.
/// </summary>
/// <param name="limit">Number of repeated messages to skip before writing the collapsed summary.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format, args, etc.</param>
#define LogmeE_Collapse(limit, ...) \
  Logme_CollapseAt(Logme::Level::LEVEL_ERROR, limit, ## __VA_ARGS__)

/// <summary>
/// Writes ERROR log message and collapses repeated messages ignoring substrings matched by a regular expression.
/// </summary>
/// <param name="ignoreRegex">Regular expression used to remove ignored substrings before comparing messages.</param>
/// <param name="limit">Number of repeated messages to skip before writing the collapsed summary.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format, args, etc.</param>
#define LogmeE_CollapseIgnore(ignoreRegex, limit, ...) \
  Logme_CollapseIgnoreAt(Logme::Level::LEVEL_ERROR, ignoreRegex, limit, ## __VA_ARGS__)

/// <summary>
/// Writes CRITICAL log message and collapses repeated messages using the formatted message text as the repeat key.
/// </summary>
/// <param name="limit">Number of repeated messages to skip before writing the collapsed summary.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format, args, etc.</param>
#define LogmeC_Collapse(limit, ...) \
  Logme_CollapseAt(Logme::Level::LEVEL_CRITICAL, limit, ## __VA_ARGS__)

/// <summary>
/// Writes CRITICAL log message and collapses repeated messages ignoring substrings matched by a regular expression.
/// </summary>
/// <param name="ignoreRegex">Regular expression used to remove ignored substrings before comparing messages.</param>
/// <param name="limit">Number of repeated messages to skip before writing the collapsed summary.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, format, args, etc.</param>
#define LogmeC_CollapseIgnore(ignoreRegex, limit, ...) \
  Logme_CollapseIgnoreAt(Logme::Level::LEVEL_CRITICAL, ignoreRegex, limit, ## __VA_ARGS__)

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

#if LOGME_ACTIVE
  #define LOGMEP_ONCE_OVR() ([]() -> Logme::Override& { static Logme::Override ovr(1); return ovr; }())
  #define LOGMEP_RATE_OVR(ms) ([]() -> Logme::Override& { static Logme::Override ovr(-1, ms); return ovr; }())
#else
  #define LOGMEP_ONCE_OVR()
  #define LOGMEP_RATE_OVR(ms)
#endif

// Log once (per call site)

#ifdef _MSC_VER
/// <summary>
/// Writes DEBUG log message once per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeD_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, LOGMEP_ONCE_OVR(), ## __VA_ARGS__)
#else
/// <summary>
/// Writes DEBUG log message once per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeD_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, LOGMEP_ONCE_OVR() LOGMEP_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
/// <summary>
/// Writes INFO log message once per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeI_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, LOGMEP_ONCE_OVR(), ## __VA_ARGS__)
#else
/// <summary>
/// Writes INFO log message once per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeI_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, LOGMEP_ONCE_OVR() LOGMEP_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
/// <summary>
/// Writes WARN log message once per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeW_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, LOGMEP_ONCE_OVR(), ## __VA_ARGS__)
#else
/// <summary>
/// Writes WARN log message once per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeW_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, LOGMEP_ONCE_OVR() LOGMEP_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
/// <summary>
/// Writes ERROR log message once per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeE_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, LOGMEP_ONCE_OVR(), ## __VA_ARGS__)
#else
/// <summary>
/// Writes ERROR log message once per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeE_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, LOGMEP_ONCE_OVR() LOGMEP_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
/// <summary>
/// Writes CRITICAL log message once per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeC_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, LOGMEP_ONCE_OVR(), ## __VA_ARGS__)
#else
/// <summary>
/// Writes CRITICAL log message once per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeC_Once(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, LOGMEP_ONCE_OVR() LOGMEP_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

// Log with interval (rate-limited)

#ifdef _MSC_VER
/// <summary>
/// Writes DEBUG log message at most once per interval per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeD_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, LOGMEP_RATE_OVR(ms), ## __VA_ARGS__)
#else
/// <summary>
/// Writes DEBUG log message at most once per interval per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeD_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, LOGMEP_RATE_OVR(ms) LOGMEP_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
/// <summary>
/// Writes INFO log message at most once per interval per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeI_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, LOGMEP_RATE_OVR(ms), ## __VA_ARGS__)
#else
/// <summary>
/// Writes INFO log message at most once per interval per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeI_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, LOGMEP_RATE_OVR(ms) LOGMEP_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
/// <summary>
/// Writes WARN log message at most once per interval per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeW_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, LOGMEP_RATE_OVR(ms), ## __VA_ARGS__)
#else
/// <summary>
/// Writes WARN log message at most once per interval per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeW_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, LOGMEP_RATE_OVR(ms) LOGMEP_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
/// <summary>
/// Writes ERROR log message at most once per interval per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeE_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, LOGMEP_RATE_OVR(ms), ## __VA_ARGS__)
#else
/// <summary>
/// Writes ERROR log message at most once per interval per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeE_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, LOGMEP_RATE_OVR(ms) LOGMEP_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

#ifdef _MSC_VER
/// <summary>
/// Writes CRITICAL log message at most once per interval per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeC_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, LOGMEP_RATE_OVR(ms), ## __VA_ARGS__)
#else
/// <summary>
/// Writes CRITICAL log message at most once per interval per call site (printf-style when called with a format string) or returns a stream (C++ style of output).
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, override, subsystem id, etc.</param>
#define LogmeC_Every(ms, ...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, LOGMEP_RATE_OVR(ms) LOGMEP_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif

// std::format

#if LOGME_ACTIVE
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
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), LOGMEP_ONCE_OVR(), ## __VA_ARGS__)

/// <summary>
/// Writes INFO message using std::format-style formatting once per call site.
/// </summary>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeI_Once(...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), LOGMEP_ONCE_OVR(), ## __VA_ARGS__)

/// <summary>
/// Writes WARN message using std::format-style formatting once per call site.
/// </summary>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeW_Once(...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), LOGMEP_ONCE_OVR(), ## __VA_ARGS__)

/// <summary>
/// Writes ERROR message using std::format-style formatting once per call site.
/// </summary>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeE_Once(...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), LOGMEP_ONCE_OVR(), ## __VA_ARGS__)

/// <summary>
/// Writes CRITICAL message using std::format-style formatting once per call site.
/// </summary>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeC_Once(...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, Logme::GetStdFormat(), LOGMEP_ONCE_OVR(), ## __VA_ARGS__)

/// <summary>
/// Writes DEBUG message using std::format-style formatting and global CH/SUBSID once per call site.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeDg_Once(...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), LOGMEP_ONCE_OVR(), ## __VA_ARGS__)

/// <summary>
/// Writes INFO message using std::format-style formatting and global CH/SUBSID once per call site.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeIg_Once(...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), LOGMEP_ONCE_OVR(), ## __VA_ARGS__)

/// <summary>
/// Writes WARN message using std::format-style formatting and global CH/SUBSID once per call site.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeWg_Once(...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), LOGMEP_ONCE_OVR(), ## __VA_ARGS__)

/// <summary>
/// Writes ERROR message using std::format-style formatting and global CH/SUBSID once per call site.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeEg_Once(...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), LOGMEP_ONCE_OVR(), ## __VA_ARGS__)

/// <summary>
/// Writes CRITICAL message using std::format-style formatting and global CH/SUBSID once per call site.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeCg_Once(...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, Logme::GetStdFormat(), LOGMEP_ONCE_OVR(), ## __VA_ARGS__)

/// <summary>
/// Writes DEBUG message using std::format-style formatting at most once per interval per call site.
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeD_Every(ms, ...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), LOGMEP_RATE_OVR(ms), ## __VA_ARGS__)

/// <summary>
/// Writes INFO message using std::format-style formatting at most once per interval per call site.
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeI_Every(ms, ...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), LOGMEP_RATE_OVR(ms), ## __VA_ARGS__)

/// <summary>
/// Writes WARN message using std::format-style formatting at most once per interval per call site.
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeW_Every(ms, ...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), LOGMEP_RATE_OVR(ms), ## __VA_ARGS__)

/// <summary>
/// Writes ERROR message using std::format-style formatting at most once per interval per call site.
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeE_Every(ms, ...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), LOGMEP_RATE_OVR(ms), ## __VA_ARGS__)

/// <summary>
/// Writes CRITICAL message using std::format-style formatting at most once per interval per call site.
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeC_Every(ms, ...) \
  fLogme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, Logme::GetStdFormat(), LOGMEP_RATE_OVR(ms), ## __VA_ARGS__)

/// <summary>
/// Writes DEBUG message using std::format-style formatting and global CH/SUBSID at most once per interval per call site.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeDg_Every(ms, ...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), LOGMEP_RATE_OVR(ms), ## __VA_ARGS__)

/// <summary>
/// Writes INFO message using std::format-style formatting and global CH/SUBSID at most once per interval per call site.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeIg_Every(ms, ...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), LOGMEP_RATE_OVR(ms), ## __VA_ARGS__)

/// <summary>
/// Writes WARN message using std::format-style formatting and global CH/SUBSID at most once per interval per call site.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeWg_Every(ms, ...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), LOGMEP_RATE_OVR(ms), ## __VA_ARGS__)

/// <summary>
/// Writes ERROR message using std::format-style formatting and global CH/SUBSID at most once per interval per call site.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeEg_Every(ms, ...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), LOGMEP_RATE_OVR(ms), ## __VA_ARGS__)

/// <summary>
/// Writes CRITICAL message using std::format-style formatting and global CH/SUBSID at most once per interval per call site.
/// Designed for static methods of a class where CH/SUBSID are declared as class members.
/// </summary>
/// <param name="ms">Minimum interval in milliseconds.</param>
/// <param name="...">Optional arguments: channel/id, subsystem id, format and format arguments.</param>
#define fLogmeCg_Every(ms, ...) \
  fLogme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, Logme::GetStdFormat(), LOGMEP_RATE_OVR(ms), ## __VA_ARGS__)


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

// Trace Points

#if LOGME_ACTIVE
#ifdef _MSC_VER
  #define LOGME_TP_BODY(level, dispatch, ...) \
    do \
    { \
      static Logme::TracePoint tracePoint(__FILE__, __FUNCTION__, __LINE__); \
      tracePoint.Hit(); \
      if (!tracePoint.Registered) \
        Logme::Detail::RegisterTracePointOnce(tracePoint); \
      if (tracePoint.Enabled && Logme::Instance->Condition()) \
      { \
        static Logme::ContextCache contextCache; \
        dispatch(Logme::Instance, contextCache, level, &CH, &SUBSID, __FUNCTION__, __FILE__, __LINE__, ## __VA_ARGS__); \
      } \
    } while (0)
#else
  #define LOGME_TP_BODY(level, dispatch, ...) \
    do \
    { \
      static Logme::TracePoint tracePoint(__FILE__, __FUNCTION__, __LINE__); \
      tracePoint.Hit(); \
      if (!tracePoint.Registered) \
        Logme::Detail::RegisterTracePointOnce(tracePoint); \
      if (tracePoint.Enabled && Logme::Instance->Condition()) \
      { \
        static Logme::ContextCache contextCache; \
        dispatch(Logme::Instance, contextCache, level, &CH, &SUBSID, __FUNCTION__, __FILE__, __LINE__ __VA_OPT__(, __VA_ARGS__)); \
      } \
    } while (0)
#endif

#ifdef _MSC_VER
  #define LogmeD_TPt(...) LOGME_TP_BODY(Logme::Level::LEVEL_DEBUG, Logme::Detail::DispatchTracePoint, ## __VA_ARGS__)
  #define LogmeI_TPt(...) LOGME_TP_BODY(Logme::Level::LEVEL_INFO, Logme::Detail::DispatchTracePoint, ## __VA_ARGS__)
  #define LogmeW_TPt(...) LOGME_TP_BODY(Logme::Level::LEVEL_WARN, Logme::Detail::DispatchTracePoint, ## __VA_ARGS__)
  #define LogmeE_TPt(...) LOGME_TP_BODY(Logme::Level::LEVEL_ERROR, Logme::Detail::DispatchTracePoint, ## __VA_ARGS__)
  #define LogmeC_TPt(...) LOGME_TP_BODY(Logme::Level::LEVEL_CRITICAL, Logme::Detail::DispatchTracePoint, ## __VA_ARGS__)
#else
  #define LogmeD_TPt(...) LOGME_TP_BODY(Logme::Level::LEVEL_DEBUG, Logme::Detail::DispatchTracePoint __VA_OPT__(, __VA_ARGS__))
  #define LogmeI_TPt(...) LOGME_TP_BODY(Logme::Level::LEVEL_INFO, Logme::Detail::DispatchTracePoint __VA_OPT__(, __VA_ARGS__))
  #define LogmeW_TPt(...) LOGME_TP_BODY(Logme::Level::LEVEL_WARN, Logme::Detail::DispatchTracePoint __VA_OPT__(, __VA_ARGS__))
  #define LogmeE_TPt(...) LOGME_TP_BODY(Logme::Level::LEVEL_ERROR, Logme::Detail::DispatchTracePoint __VA_OPT__(, __VA_ARGS__))
  #define LogmeC_TPt(...) LOGME_TP_BODY(Logme::Level::LEVEL_CRITICAL, Logme::Detail::DispatchTracePoint __VA_OPT__(, __VA_ARGS__))
#endif

#define LogmeTPt(...) LogmeI_TPt(__VA_ARGS__)

#ifndef LOGME_DISABLE_STD_FORMAT
#ifdef _MSC_VER
  #define fLogmeD_TPt(...) LOGME_TP_BODY(Logme::Level::LEVEL_DEBUG, Logme::Detail::DispatchTracePointStdFormat, Logme::GetStdFormat(), ## __VA_ARGS__)
  #define fLogmeI_TPt(...) LOGME_TP_BODY(Logme::Level::LEVEL_INFO, Logme::Detail::DispatchTracePointStdFormat, Logme::GetStdFormat(), ## __VA_ARGS__)
  #define fLogmeW_TPt(...) LOGME_TP_BODY(Logme::Level::LEVEL_WARN, Logme::Detail::DispatchTracePointStdFormat, Logme::GetStdFormat(), ## __VA_ARGS__)
  #define fLogmeE_TPt(...) LOGME_TP_BODY(Logme::Level::LEVEL_ERROR, Logme::Detail::DispatchTracePointStdFormat, Logme::GetStdFormat(), ## __VA_ARGS__)
  #define fLogmeC_TPt(...) LOGME_TP_BODY(Logme::Level::LEVEL_CRITICAL, Logme::Detail::DispatchTracePointStdFormat, Logme::GetStdFormat(), ## __VA_ARGS__)
#else
  #define fLogmeD_TPt(...) LOGME_TP_BODY(Logme::Level::LEVEL_DEBUG, Logme::Detail::DispatchTracePointStdFormat, Logme::GetStdFormat() __VA_OPT__(, __VA_ARGS__))
  #define fLogmeI_TPt(...) LOGME_TP_BODY(Logme::Level::LEVEL_INFO, Logme::Detail::DispatchTracePointStdFormat, Logme::GetStdFormat() __VA_OPT__(, __VA_ARGS__))
  #define fLogmeW_TPt(...) LOGME_TP_BODY(Logme::Level::LEVEL_WARN, Logme::Detail::DispatchTracePointStdFormat, Logme::GetStdFormat() __VA_OPT__(, __VA_ARGS__))
  #define fLogmeE_TPt(...) LOGME_TP_BODY(Logme::Level::LEVEL_ERROR, Logme::Detail::DispatchTracePointStdFormat, Logme::GetStdFormat() __VA_OPT__(, __VA_ARGS__))
  #define fLogmeC_TPt(...) LOGME_TP_BODY(Logme::Level::LEVEL_CRITICAL, Logme::Detail::DispatchTracePointStdFormat, Logme::GetStdFormat() __VA_OPT__(, __VA_ARGS__))
#endif
#define fLogmeTPt(...) fLogmeI_TPt(__VA_ARGS__)
#else
  #define fLogmeD_TPt(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)
  #define fLogmeI_TPt(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)
  #define fLogmeW_TPt(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)
  #define fLogmeE_TPt(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)
  #define fLogmeC_TPt(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)
  #define fLogmeTPt(...) do { static_assert(false, "logme: fLogme* macros require std::format support. Enable LOGME_STD_FORMAT=ON/AUTO."); } while (0)
#endif
#else
  #define LogmeD_TPt(...)
  #define LogmeI_TPt(...)
  #define LogmeW_TPt(...)
  #define LogmeE_TPt(...)
  #define LogmeC_TPt(...)
  #define LogmeTPt(...)
  #define fLogmeD_TPt(...)
  #define fLogmeI_TPt(...)
  #define fLogmeW_TPt(...)
  #define fLogmeE_TPt(...)
  #define fLogmeC_TPt(...)
  #define fLogmeTPt(...)
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

// Thread subsystem

/// <summary>Sets the current thread subsystem for the current scope.</summary>
/// <param name="sid">Subsystem id used by log records from this scope.</param>
#define LogmeThreadSubsystem(sid) \
  Logme::ThreadSubsystem _logme_thread_subsystem(Logme::Instance, sid)

/// <summary>Returns true when the current thread has an explicitly assigned subsystem.</summary>
#define LogmeThreadSubsystemDefined() \
  Logme::Instance->IsSubsystemDefinedForCurrentThread()

#endif
