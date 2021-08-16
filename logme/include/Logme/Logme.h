#pragma once

#include <Logme/Channel.h>
#include <Logme/Convert.h>
#include <Logme/Logger.h>
#include <Logme/Procedure.h>
#include <Logme/Stream.h>

// String conversion

#define _S(str) Logme::ToStdString(str)
#define _WS(str) Logme::ToStdWString(str)

// C-style logging

#if _LOGME_ACTIVE
#define _Logme_If(condition, logger, level, ...) \
  if ((condition)) \
    logger->Log(LOGME_CONTEXT(level, &CH), ## __VA_ARGS__)
#else
#define _Logme_If(condition, logger, level, ...)
#endif

#define LogmeD(...) \
  _Logme_If(true, Logme::Instance, Logme::Level::LEVEL_DEBUG, ## __VA_ARGS__)

#define LogmeD_If(condition, ...) \
  _Logme_If(condition, Logme::Instance, Logme::Level::LEVEL_DEBUG, ## __VA_ARGS__)

#define LogmeI(...) \
  _Logme_If(true, Logme::Instance, Logme::Level::LEVEL_INFO, ## __VA_ARGS__)

#define LogmeI_If(condition, ...) \
  _Logme_If(condition, Logme::Instance, Logme::Level::LEVEL_INFO, ## __VA_ARGS__)

#define LogmeW(...) \
  _Logme_If(true, Logme::Instance, Logme::Level::LEVEL_WARN, ## __VA_ARGS__)

#define LogmeW_If(condition, ...) \
  _Logme_If(condition, Logme::Instance, Logme::Level::LEVEL_WARN, ## __VA_ARGS__)

#define LogmeE(...) \
  _Logme_If(true, Logme::Instance, Logme::Level::LEVEL_ERROR, ## __VA_ARGS__)

#define LogmeE_If(condition, ...) \
  _Logme_If(condition, Logme::Instance, Logme::Level::LEVEL_ERROR, ## __VA_ARGS__)

#define LogmeC(...) \
  _Logme_If(true, Logme::Instance, Logme::Level::LEVEL_CRITICAL, ## __VA_ARGS__)

#define LogmeC_If(condition, ...) \
  _Logme_If(condition, Logme::Instance, Logme::Level::LEVEL_CRITICAL, ## __VA_ARGS__)

// C++ style

#if _LOGME_ACTIVE
#define _logme(logger, level, ...) \
  Logme::Stream(logger, LOGME_CONTEXT(level, &CH, ## __VA_ARGS__))
#else
#define _logme(logger, level, ...) \
  std::stringstream()
#endif

#define logme(...) \
  _logme(Logme::Instance, Logme::DEFAULT_LEVEL, ## __VA_ARGS__)

#define logmeD(...) \
  _logme(Logme::Instance, Logme::Level::LEVEL_DEBUG, ## __VA_ARGS__)

#define logmeI(...) \
  _logme(Logme::Instance, Logme::Level::LEVEL_INFO, ## __VA_ARGS__)

#define logmeW(...) \
  _logme(Logme::Instance, Logme::Level::LEVEL_WARN, ## __VA_ARGS__)

#define logmeE(...) \
  _logme(Logme::Instance, Logme::Level::LEVEL_ERROR, ## __VA_ARGS__)

#define logmeC(...) \
  _logme(Logme::Instance, Logme::Level::LEVEL_CRITICAL, ## __VA_ARGS__)


#if _LOGME_ACTIVE
#define _wlogme(logger, level, ...) \
  Logme::WStream(logger, LOGME_CONTEXT(level, &CH, ## __VA_ARGS__))
#else
#define _wlogme(logger, level, ...) \
  std::wstringstream()
#endif

#define wlogme(...) \
  _wlogme(Logme::Instance, Logme::DEFAULT_LEVEL, ## __VA_ARGS__)

#define wlogmeD(...) \
  _wlogme(Logme::Instance, Logme::Level::LEVEL_DEBUG, ## __VA_ARGS__)

#define wlogmeI(...) \
  _wlogme(Logme::Instance, Logme::Level::LEVEL_INFO, ## __VA_ARGS__)

#define wlogmeW(...) \
  _wlogme(Logme::Instance, Logme::Level::LEVEL_WARN, ## __VA_ARGS__)

#define wlogmeE(...) \
  _wlogme(Logme::Instance, Logme::Level::LEVEL_ERROR, ## __VA_ARGS__)

#define wlogmeC(...) \
  _wlogme(Logme::Instance, Logme::Level::LEVEL_CRITICAL, ## __VA_ARGS__)
