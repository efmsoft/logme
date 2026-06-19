#pragma once

#include <sstream>

#include <Logme/Logme.h>

#ifndef LOGME_GLOG_VLOG_LEVEL
  #define LOGME_GLOG_VLOG_LEVEL 0
#endif

#define LOGME_GLOG_CAT_INNER(left, right) left##right
#define LOGME_GLOG_CAT(left, right) LOGME_GLOG_CAT_INNER(left, right)

#define LOGME_GLOG_LOG_INFO() LogmeI()
#define LOGME_GLOG_LOG_WARNING() LogmeW()
#define LOGME_GLOG_LOG_ERROR() LogmeE()
#define LOGME_GLOG_LOG_FATAL() LogmeC()
#define LOGME_GLOG_LOG(severity) LOGME_GLOG_CAT(LOGME_GLOG_LOG_, severity)()

#define LOG(severity) LOGME_GLOG_LOG(severity)
#define LOG_IF(severity, condition) \
  if (!(condition)) \
  { \
  } \
  else \
    LOG(severity)

#define LOGME_GLOG_PLOG_INFO() LogmeI_P()
#define LOGME_GLOG_PLOG_WARNING() LogmeW_P()
#define LOGME_GLOG_PLOG_ERROR() LogmeE_P()
#define LOGME_GLOG_PLOG_FATAL() LogmeC_P()
#define LOGME_GLOG_PLOG(severity) LOGME_GLOG_CAT(LOGME_GLOG_PLOG_, severity)()

#define PLOG(severity) LOGME_GLOG_PLOG(severity)
#define PLOG_IF(severity, condition) \
  if (!(condition)) \
  { \
  } \
  else \
    PLOG(severity)

#define CHECK(condition) LogmeCheck(condition)
#define CHECK_EQ(left, right) LogmeCheckEq(left, right)
#define CHECK_NE(left, right) LogmeCheckNe(left, right)
#define CHECK_LT(left, right) LogmeCheckLt(left, right)
#define CHECK_LE(left, right) LogmeCheckLe(left, right)
#define CHECK_GT(left, right) LogmeCheckGt(left, right)
#define CHECK_GE(left, right) LogmeCheckGe(left, right)
#define CHECK_NOTNULL(pointer) LogmeCheckNotNull(pointer)
#define PCHECK(condition) LogmePCheck(condition)

#ifndef NDEBUG
  #define DLOG(severity) LOG(severity)
  #define DLOG_IF(severity, condition) LOG_IF(severity, condition)
  #define DCHECK(condition) CHECK(condition)
  #define DCHECK_EQ(left, right) CHECK_EQ(left, right)
  #define DCHECK_NE(left, right) CHECK_NE(left, right)
  #define DCHECK_LT(left, right) CHECK_LT(left, right)
  #define DCHECK_LE(left, right) CHECK_LE(left, right)
  #define DCHECK_GT(left, right) CHECK_GT(left, right)
  #define DCHECK_GE(left, right) CHECK_GE(left, right)
  #define DCHECK_NOTNULL(pointer) CHECK_NOTNULL(pointer)
#else
  #define DLOG(severity) if (true) { } else std::stringstream()
  #define DLOG_IF(severity, condition) if (true) { } else std::stringstream()
  #define DCHECK(condition) if (true) { } else std::stringstream()
  #define DCHECK_EQ(left, right) if (true) { } else std::stringstream()
  #define DCHECK_NE(left, right) if (true) { } else std::stringstream()
  #define DCHECK_LT(left, right) if (true) { } else std::stringstream()
  #define DCHECK_LE(left, right) if (true) { } else std::stringstream()
  #define DCHECK_GT(left, right) if (true) { } else std::stringstream()
  #define DCHECK_GE(left, right) if (true) { } else std::stringstream()
  #define DCHECK_NOTNULL(pointer) (pointer)
#endif

#define VLOG_IS_ON(verboseLevel) ((verboseLevel) <= LOGME_GLOG_VLOG_LEVEL)
#define VLOG(verboseLevel) \
  if (!VLOG_IS_ON(verboseLevel)) \
  { \
  } \
  else \
    LogmeI()
#define VLOG_IF(verboseLevel, condition) \
  if (!(condition) || !VLOG_IS_ON(verboseLevel)) \
  { \
  } \
  else \
    LogmeI()

#define LOGME_GLOG_FIRST_N_INFO(count) LogmeI_FirstN(count)
#define LOGME_GLOG_FIRST_N_WARNING(count) LogmeW_FirstN(count)
#define LOGME_GLOG_FIRST_N_ERROR(count) LogmeE_FirstN(count)
#define LOGME_GLOG_FIRST_N_FATAL(count) LogmeC_FirstN(count)
#define LOGME_GLOG_FIRST_N(severity, count) LOGME_GLOG_CAT(LOGME_GLOG_FIRST_N_, severity)(count)

#define LOGME_GLOG_EVERY_N_INFO(count) LogmeI_EveryN(count)
#define LOGME_GLOG_EVERY_N_WARNING(count) LogmeW_EveryN(count)
#define LOGME_GLOG_EVERY_N_ERROR(count) LogmeE_EveryN(count)
#define LOGME_GLOG_EVERY_N_FATAL(count) LogmeC_EveryN(count)
#define LOGME_GLOG_EVERY_N(severity, count) LOGME_GLOG_CAT(LOGME_GLOG_EVERY_N_, severity)(count)

#define LOG_FIRST_N(severity, count) LOGME_GLOG_FIRST_N(severity, count)
#define LOG_EVERY_N(severity, count) LOGME_GLOG_EVERY_N(severity, count)
