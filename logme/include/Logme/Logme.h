#pragma once

#include <Logme/AllStatI.h>
#include <Logme/ArgumentList.h>
#include <Logme/Channel.h>
#include <Logme/Convert.h>
#include <Logme/Logger.h>
#include <Logme/Procedure.h>
#include <Logme/Stream.h>
#include <Logme/ThreadChannel.h>
#include <Logme/ThreadOverride.h>

// String conversion

#define _S(str) Logme::ToStdString(str)
#define _WS(str) Logme::ToStdWString(str)

// C/C++ - style logging

#if _LOGME_ACTIVE
#ifdef _MSC_VER
  #define Logme_If(condition, logger, level, ...) \
    if ((condition)) \
      logger->Log(LOGME_CONTEXT(level, &CH, __VA_ARGS__) , __VA_ARGS__)
  #define Logme_Ifg(condition, logger, level, ...) \
    if ((condition)) \
      logger->Log(LOGME_CONTEXT(level, &::CH, __VA_ARGS__) , __VA_ARGS__)
#else
  #define Logme_If(condition, logger, level, ...) \
    if ((condition)) \
      logger->Log(LOGME_CONTEXT(level, &CH) _LOGME_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
  #define Logme_Ifg(condition, logger, level, ...) \
    if ((condition)) \
      logger->Log(LOGME_CONTEXT(level, &::CH) _LOGME_NONEMPTY(__VA_ARGS__) __VA_ARGS__)
#endif
#else
  #define Logme_If(condition, logger, level, ...)
  #define Logme_Ifg(condition, logger, level, ...)
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

// std::format

#define fLogmeD(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeD_If(condition, ...) \
  Logme_If(condition, Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeI(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeI_If(condition, ...) \
  Logme_If(condition, Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeW(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeW_If(condition, ...) \
  Logme_If(condition, Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeE(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeE_If(condition, ...) \
  Logme_If(condition, Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeC(...) \
  Logme_If(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeC_If(condition, ...) \
  Logme_If(condition, Logme::Instance, Logme::Level::LEVEL_CRITICAL, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeDg(...) \
  Logme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeDg_If(condition, ...) \
  Logme_Ifg(condition, Logme::Instance, Logme::Level::LEVEL_DEBUG, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeIg(...) \
  Logme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeI_Ifg(condition, ...) \
  Logme_Ifg(condition, Logme::Instance, Logme::Level::LEVEL_INFO, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeWg(...) \
  Logme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeW_Ifg(condition, ...) \
  Logme_Ifg(condition, Logme::Instance, Logme::Level::LEVEL_WARN, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeEg(...) \
  Logme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeE_Ifg(condition, ...) \
  Logme_Ifg(condition, Logme::Instance, Logme::Level::LEVEL_ERROR, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeCg(...) \
  Logme_Ifg(Logme::Instance->Condition(), Logme::Instance, Logme::Level::LEVEL_CRITICAL, Logme::GetStdFormat(), ## __VA_ARGS__)

#define fLogmeC_Ifg(condition, ...) \
  Logme_Ifg(condition, Logme::Instance, Logme::Level::LEVEL_CRITICAL, Logme::GetStdFormat(), ## __VA_ARGS__)

// Thread channel 

#define LogmeThreadChannel(ch) \
  Logme::ThreadChannel _logme_thread_channel(Logme::Instance, ch)

// Thread override

#define LogmeThreadOverride(ovr) \
  Logme::ThreadOverride _logme_thread_override(Logme::Instance, ovr)
