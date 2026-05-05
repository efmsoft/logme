#pragma once

#include <chrono>
#include <stdarg.h>
#include <string>

#include <Logme/ID.h>
#include <Logme/Printer.h>
#include <Logme/Types.h>

namespace Logme
{
  class Procedure
  {
  protected:
    const Context& BeginContext;
    const ID& Channel;
    Printer* RValPrinter;
    std::string Duration;
    
    std::chrono::time_point<std::chrono::system_clock> Begin;

  public:
    LOGMELNK Procedure(
      const Context& context
      , Printer* printer
      , const ID& ch
      , const char* format = nullptr
      , ...
    );

    LOGMELNK Procedure(
      const Context& context
      , Printer* printer
      , const ChannelPtr& pch
      , const char* format = nullptr
      , ...
    );

    LOGMELNK Procedure(
      const Context& context
      , Printer* printer
      , const char* format = nullptr
      , ...
    );

    LOGMELNK virtual ~Procedure();

    LOGMELNK void Print(bool begin, const char* text = nullptr);
    LOGMELNK static std::string Format(const char* format, va_list args);
    LOGMELNK static const char* AppendDuration(struct Context& context);
  };
}

#define NoRetval  Logme::None

#if LOGME_ACTIVE

#if defined(__CLION_IDE__) || defined(__JETBRAINS_IDE__)
#define LOGMEP_LogmeP(level, retval, ...) \
  unsigned char _procStorage[sizeof(Logme::PrinterT<int>)]; \
  static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); \
  const Logme::Context& _procContext = LOGME_CONTEXT(LOGME_JOIN(_logme_ctx_, __LINE__), level, &CH, &SUBSID __VA_OPT__(,) __VA_ARGS__); \
  Logme::Procedure logme_proc(_procContext, Logme::CreatePrinter(retval, _procStorage) __VA_OPT__(,) __VA_ARGS__)

#define LOGMEP_LogmePV(level, ...) \
  static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); \
  const Logme::Context& _procContext = LOGME_CONTEXT(LOGME_JOIN(_logme_ctx_, __LINE__), level, &CH, &SUBSID __VA_OPT__(,) __VA_ARGS__); \
  Logme::Procedure logme_proc(_procContext, nullptr __VA_OPT__(,) __VA_ARGS__)
#elif defined(__cplusplus) && (__cplusplus >= 202002L) && defined(__clang__)
#define LOGMEP_LogmeP(level, retval, ...) \
  unsigned char _procStorage[sizeof(Logme::PrinterT<int>)]; \
  static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); \
  const Logme::Context& _procContext = LOGME_CONTEXT(LOGME_JOIN(_logme_ctx_, __LINE__), level, &CH, &SUBSID __VA_OPT__(,) __VA_ARGS__); \
  Logme::Procedure logme_proc(_procContext, Logme::CreatePrinter(retval, _procStorage) __VA_OPT__(,) __VA_ARGS__)

#define LOGMEP_LogmePV(level, ...) \
  static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); \
  const Logme::Context& _procContext = LOGME_CONTEXT(LOGME_JOIN(_logme_ctx_, __LINE__), level, &CH, &SUBSID __VA_OPT__(,) __VA_ARGS__); \
  Logme::Procedure logme_proc(_procContext, nullptr LOGMEP_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#elif defined(__GNUC__)
  #define LOGMEP_LogmeP(level, retval, ...) \
    unsigned char _procStorage[sizeof(Logme::PrinterT<int>)]; \
    static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); \
    const Logme::Context& _procContext = LOGME_CONTEXT(LOGME_JOIN(_logme_ctx_, __LINE__), level, &CH, &SUBSID, ## __VA_ARGS__); \
    Logme::Procedure logme_proc(_procContext, Logme::CreatePrinter(retval, _procStorage), ## __VA_ARGS__)

  #define LOGMEP_LogmePV(level, ...) \
    static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); \
    const Logme::Context& _procContext = LOGME_CONTEXT(LOGME_JOIN(_logme_ctx_, __LINE__), level, &CH, &SUBSID, ## __VA_ARGS__); \
    Logme::Procedure logme_proc(_procContext, nullptr LOGMEP_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#else
#define LOGMEP_LogmeP(level, retval, ...) \
  unsigned char _procStorage[sizeof(Logme::PrinterT<int>)]; \
  static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); \
  const Logme::Context& _procContext = LOGME_CONTEXT(LOGME_JOIN(_logme_ctx_, __LINE__), level, &CH, &SUBSID, ## __VA_ARGS__); \
  Logme::Procedure logme_proc(_procContext, Logme::CreatePrinter(retval, _procStorage), ## __VA_ARGS__)

#define LOGMEP_LogmePV(level, ...) \
  static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); \
  const Logme::Context& _procContext = LOGME_CONTEXT(LOGME_JOIN(_logme_ctx_, __LINE__), level, &CH, &SUBSID, ## __VA_ARGS__); \
  Logme::Procedure logme_proc(_procContext, nullptr, ## __VA_ARGS__)
#endif 
#else
#define LOGMEP_LogmeP(level, retval, ...)
#define LOGMEP_LogmePV(level, ...)
#endif

#if defined(__cplusplus) && (__cplusplus >= 202002L) && defined(__clang__)
/// <summary>
/// Logs procedure begin/end at INFO level and prints retval on exit.
/// </summary>
/// <param name="retval">Return value variable printed when the procedure scope ends.</param>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmeP(retval, ...) \
  LOGMEP_LogmeP(Logme::Level::LEVEL_INFO, retval __VA_OPT__(,) __VA_ARGS__)

/// <summary>
/// Logs procedure begin/end at INFO level without return value.
/// </summary>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmePV(...) \
  LOGMEP_LogmePV(Logme::Level::LEVEL_INFO __VA_OPT__(,) __VA_ARGS__)

/// <summary>
/// Logs procedure begin/end at INFO level and prints retval on exit.
/// </summary>
/// <param name="retval">Return value variable printed when the procedure scope ends.</param>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmePI(retval, ...) \
  LOGMEP_LogmeP(Logme::Level::LEVEL_INFO, retval __VA_OPT__(,) __VA_ARGS__)

/// <summary>
/// Logs procedure begin/end at INFO level without return value.
/// </summary>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmePVI(...) \
  LOGMEP_LogmePV(Logme::Level::LEVEL_INFO __VA_OPT__(,) __VA_ARGS__)

/// <summary>
/// Logs procedure begin/end at DEBUG level and prints retval on exit.
/// </summary>
/// <param name="retval">Return value variable printed when the procedure scope ends.</param>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmePD(retval, ...) \
  LOGMEP_LogmeP(Logme::Level::LEVEL_DEBUG, retval __VA_OPT__(,) __VA_ARGS__)

/// <summary>
/// Logs procedure begin/end at DEBUG level without return value.
/// </summary>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmePVD(...) \
  LOGMEP_LogmePV(Logme::Level::LEVEL_DEBUG __VA_OPT__(,) __VA_ARGS__)

/// <summary>
/// Logs procedure begin/end at WARN level and prints retval on exit.
/// </summary>
/// <param name="retval">Return value variable printed when the procedure scope ends.</param>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmePW(retval, ...) \
  LOGMEP_LogmeP(Logme::Level::LEVEL_WARN, retval __VA_OPT__(,) __VA_ARGS__)

/// <summary>
/// Logs procedure begin/end at WARN level without return value.
/// </summary>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmePVW(...) \
  LOGMEP_LogmePV(Logme::Level::LEVEL_WARN __VA_OPT__(,) __VA_ARGS__)

/// <summary>
/// Logs procedure begin/end at ERROR level and prints retval on exit.
/// </summary>
/// <param name="retval">Return value variable printed when the procedure scope ends.</param>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmePE(retval, ...) \
  LOGMEP_LogmeP(Logme::Level::LEVEL_ERROR, retval __VA_OPT__(,) __VA_ARGS__)

/// <summary>
/// Logs procedure begin/end at ERROR level without return value.
/// </summary>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmePVE(...) \
  LOGMEP_LogmePV(Logme::Level::LEVEL_ERROR __VA_OPT__(,) __VA_ARGS__)

/// <summary>
/// Logs procedure begin/end at CRITICAL level and prints retval on exit.
/// </summary>
/// <param name="retval">Return value variable printed when the procedure scope ends.</param>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmePC(retval, ...) \
  LOGMEP_LogmeP(Logme::Level::LEVEL_CRITICAL, retval __VA_OPT__(,) __VA_ARGS__)

/// <summary>
/// Logs procedure begin/end at CRITICAL level without return value.
/// </summary>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmePVC(...) \
  LOGMEP_LogmePV(Logme::Level::LEVEL_CRITICAL __VA_OPT__(,) __VA_ARGS__)

#else

/// <summary>
/// Logs procedure begin/end at INFO level and prints retval on exit.
/// </summary>
/// <param name="retval">Return value variable printed when the procedure scope ends.</param>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmeP(retval, ...) \
  LOGMEP_LogmeP(Logme::Level::LEVEL_INFO, retval, ## __VA_ARGS__)

/// <summary>
/// Logs procedure begin/end at INFO level without return value.
/// </summary>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmePV(...) \
  LOGMEP_LogmePV(Logme::Level::LEVEL_INFO, ## __VA_ARGS__)

/// <summary>
/// Logs procedure begin/end at INFO level and prints retval on exit.
/// </summary>
/// <param name="retval">Return value variable printed when the procedure scope ends.</param>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmePI(retval, ...) \
  LOGMEP_LogmeP(Logme::Level::LEVEL_INFO, retval, ## __VA_ARGS__)

/// <summary>
/// Logs procedure begin/end at INFO level without return value.
/// </summary>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmePVI(...) \
  LOGMEP_LogmePV(Logme::Level::LEVEL_INFO, ## __VA_ARGS__)

/// <summary>
/// Logs procedure begin/end at DEBUG level and prints retval on exit.
/// </summary>
/// <param name="retval">Return value variable printed when the procedure scope ends.</param>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmePD(retval, ...) \
  LOGMEP_LogmeP(Logme::Level::LEVEL_DEBUG, retval, ## __VA_ARGS__)

/// <summary>
/// Logs procedure begin/end at DEBUG level without return value.
/// </summary>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmePVD(...) \
  LOGMEP_LogmePV(Logme::Level::LEVEL_DEBUG, ## __VA_ARGS__)

/// <summary>
/// Logs procedure begin/end at WARN level and prints retval on exit.
/// </summary>
/// <param name="retval">Return value variable printed when the procedure scope ends.</param>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmePW(retval, ...) \
  LOGMEP_LogmeP(Logme::Level::LEVEL_WARN, retval, ## __VA_ARGS__)

/// <summary>
/// Logs procedure begin/end at WARN level without return value.
/// </summary>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmePVW(...) \
  LOGMEP_LogmePV(Logme::Level::LEVEL_WARN, ## __VA_ARGS__)

/// <summary>
/// Logs procedure begin/end at ERROR level and prints retval on exit.
/// </summary>
/// <param name="retval">Return value variable printed when the procedure scope ends.</param>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmePE(retval, ...) \
  LOGMEP_LogmeP(Logme::Level::LEVEL_ERROR, retval, ## __VA_ARGS__)

/// <summary>
/// Logs procedure begin/end at ERROR level without return value.
/// </summary>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmePVE(...) \
  LOGMEP_LogmePV(Logme::Level::LEVEL_ERROR, ## __VA_ARGS__)

/// <summary>
/// Logs procedure begin/end at CRITICAL level and prints retval on exit.
/// </summary>
/// <param name="retval">Return value variable printed when the procedure scope ends.</param>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmePC(retval, ...) \
  LOGMEP_LogmeP(Logme::Level::LEVEL_CRITICAL, retval, ## __VA_ARGS__)

/// <summary>
/// Logs procedure begin/end at CRITICAL level without return value.
/// </summary>
/// <param name="...">Optional arguments: channel/id or ChannelPtr, format and format arguments.</param>
#define LogmePVC(...) \
  LOGMEP_LogmePV(Logme::Level::LEVEL_CRITICAL, ## __VA_ARGS__)

#endif
