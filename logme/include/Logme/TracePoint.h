#pragma once

#include <atomic>
#include <stdint.h>
#include <string>

#include <Logme/Detail/Dispatch.h>
#include <Logme/Logger.h>

namespace Logme
{
  struct TracePoint
  {
    const char* Module;
    const char* Method;
    int Line;

    std::atomic<uint64_t> Counter;
    bool Enabled;
    bool Registered;
    TracePoint* Next;

    TracePoint(
      const char* module
      , const char* method
      , int line
    )
      : Module(module)
      , Method(method)
      , Line(line)
      , Counter(0)
      , Enabled(false)
      , Registered(false)
      , Next(nullptr)
    {
    }

    void Hit()
    {
      Counter.fetch_add(1, std::memory_order_relaxed);
    }

    uint64_t GetCounter() const
    {
      return Counter.load(std::memory_order_relaxed);
    }

    void ResetCounter()
    {
      Counter.store(0, std::memory_order_relaxed);
    }
  };

  namespace Detail
  {
    LOGMELNK void RegisterTracePointOnce(TracePoint& point);
    LOGMELNK bool TracePointMatch(const TracePoint& point, const std::string& pattern);
    LOGMELNK Override& TracePointOverride();

    template<typename LoggerType>
    inline void DispatchTracePoint(
      LoggerType&& logger
      , ContextCache& cache
      , Level level
      , const ID* ch
      , const SID* sid
      , const char* method
      , const char* file
      , int line
    )
    {
      Context context(
        cache
        , level
        , ch
        , sid
        , method
        , file
        , line
        , Context::Params()
      );

      logger->Log(context, TracePointOverride(), "");
    }

    template<typename LoggerType>
    inline void DispatchTracePoint(
      LoggerType&& logger
      , ContextCache& cache
      , Level level
      , const ID* ch
      , const SID* sid
      , const char* method
      , const char* file
      , int line
      , const ID& channel
    )
    {
      Context context(
        cache
        , level
        , ch
        , sid
        , method
        , file
        , line
        , Context::Params(channel)
      );

      logger->Log(context, TracePointOverride(), channel, "");
    }

    template<typename LoggerType>
    inline void DispatchTracePoint(
      LoggerType&& logger
      , ContextCache& cache
      , Level level
      , const ID* ch
      , const SID* sid
      , const char* method
      , const char* file
      , int line
      , const ChannelPtr& channel
    )
    {
      Context context(
        cache
        , level
        , ch
        , sid
        , method
        , file
        , line
        , Context::Params(channel)
      );

      logger->Log(context, TracePointOverride(), channel, "");
    }

    template<typename LoggerType>
    inline void DispatchTracePoint(
      LoggerType&& logger
      , ContextCache& cache
      , Level level
      , const ID* ch
      , const SID* sid
      , const char* method
      , const char* file
      , int line
      , const SID& subsystem
    )
    {
      Context context(
        cache
        , level
        , ch
        , sid
        , method
        , file
        , line
        , Context::Params(subsystem)
      );

      logger->Log(context, TracePointOverride(), subsystem, "");
    }

    template<typename LoggerType>
    inline void DispatchTracePoint(
      LoggerType&& logger
      , ContextCache& cache
      , Level level
      , const ID* ch
      , const SID* sid
      , const char* method
      , const char* file
      , int line
      , const ID& channel
      , const SID& subsystem
    )
    {
      Context context(
        cache
        , level
        , ch
        , sid
        , method
        , file
        , line
        , Context::Params(channel, subsystem)
      );

      logger->Log(context, TracePointOverride(), channel, subsystem, "");
    }

    template<typename LoggerType>
    inline void DispatchTracePoint(
      LoggerType&& logger
      , ContextCache& cache
      , Level level
      , const ID* ch
      , const SID* sid
      , const char* method
      , const char* file
      , int line
      , const ChannelPtr& channel
      , const SID& subsystem
    )
    {
      Context context(
        cache
        , level
        , ch
        , sid
        , method
        , file
        , line
        , Context::Params(channel, subsystem)
      );

      logger->Log(context, TracePointOverride(), channel, subsystem, "");
    }

    template<typename LoggerType>
    inline void DispatchTracePoint(
      LoggerType&& logger
      , ContextCache& cache
      , Level level
      , const ID* ch
      , const SID* sid
      , const char* method
      , const char* file
      , int line
      , ID& channel
    )
    {
      DispatchTracePoint(
        std::forward<LoggerType>(logger)
        , cache
        , level
        , ch
        , sid
        , method
        , file
        , line
        , static_cast<const ID&>(channel)
      );
    }

    template<typename LoggerType>
    inline void DispatchTracePoint(
      LoggerType&& logger
      , ContextCache& cache
      , Level level
      , const ID* ch
      , const SID* sid
      , const char* method
      , const char* file
      , int line
      , ChannelPtr& channel
    )
    {
      DispatchTracePoint(
        std::forward<LoggerType>(logger)
        , cache
        , level
        , ch
        , sid
        , method
        , file
        , line
        , static_cast<const ChannelPtr&>(channel)
      );
    }

    template<typename LoggerType>
    inline void DispatchTracePoint(
      LoggerType&& logger
      , ContextCache& cache
      , Level level
      , const ID* ch
      , const SID* sid
      , const char* method
      , const char* file
      , int line
      , SID& subsystem
    )
    {
      DispatchTracePoint(
        std::forward<LoggerType>(logger)
        , cache
        , level
        , ch
        , sid
        , method
        , file
        , line
        , static_cast<const SID&>(subsystem)
      );
    }

    template<typename LoggerType>
    inline void DispatchTracePoint(
      LoggerType&& logger
      , ContextCache& cache
      , Level level
      , const ID* ch
      , const SID* sid
      , const char* method
      , const char* file
      , int line
      , ID& channel
      , SID& subsystem
    )
    {
      DispatchTracePoint(
        std::forward<LoggerType>(logger)
        , cache
        , level
        , ch
        , sid
        , method
        , file
        , line
        , static_cast<const ID&>(channel)
        , static_cast<const SID&>(subsystem)
      );
    }

    template<typename LoggerType>
    inline void DispatchTracePoint(
      LoggerType&& logger
      , ContextCache& cache
      , Level level
      , const ID* ch
      , const SID* sid
      , const char* method
      , const char* file
      , int line
      , ChannelPtr& channel
      , SID& subsystem
    )
    {
      DispatchTracePoint(
        std::forward<LoggerType>(logger)
        , cache
        , level
        , ch
        , sid
        , method
        , file
        , line
        , static_cast<const ChannelPtr&>(channel)
        , static_cast<const SID&>(subsystem)
      );
    }

    template<typename LoggerType, typename... Args>
    inline void DispatchTracePoint(
      LoggerType&& logger
      , ContextCache& cache
      , Level level
      , const ID* ch
      , const SID* sid
      , const char* method
      , const char* file
      , int line
      , Args&&... args
    )
    {
      Context context(
        cache
        , level
        , ch
        , sid
        , method
        , file
        , line
        , MakeParams(args...)
      );

      logger->Log(context, TracePointOverride(), std::forward<Args>(args)...);
    }

#ifndef LOGME_DISABLE_STD_FORMAT
    template<typename LoggerType, typename StdFormatType>
    inline void DispatchTracePointStdFormat(
      LoggerType&& logger
      , ContextCache& cache
      , Level level
      , const ID* ch
      , const SID* sid
      , const char* method
      , const char* file
      , int line
      , StdFormatType&& stdFormat
    )
    {
      (void)stdFormat;

      Context context(
        cache
        , level
        , ch
        , sid
        , method
        , file
        , line
        , Context::Params()
      );

      logger->Log(context, TracePointOverride(), "");
    }

    template<typename LoggerType, typename StdFormatType>
    inline void DispatchTracePointStdFormat(
      LoggerType&& logger
      , ContextCache& cache
      , Level level
      , const ID* ch
      , const SID* sid
      , const char* method
      , const char* file
      , int line
      , StdFormatType&& stdFormat
      , ID& channel
    )
    {
      (void)stdFormat;

      DispatchTracePoint(
        std::forward<LoggerType>(logger)
        , cache
        , level
        , ch
        , sid
        , method
        , file
        , line
        , channel
      );
    }

    template<typename LoggerType, typename StdFormatType>
    inline void DispatchTracePointStdFormat(
      LoggerType&& logger
      , ContextCache& cache
      , Level level
      , const ID* ch
      , const SID* sid
      , const char* method
      , const char* file
      , int line
      , StdFormatType&& stdFormat
      , ChannelPtr& channel
    )
    {
      (void)stdFormat;

      DispatchTracePoint(
        std::forward<LoggerType>(logger)
        , cache
        , level
        , ch
        , sid
        , method
        , file
        , line
        , channel
      );
    }

    template<typename LoggerType, typename StdFormatType>
    inline void DispatchTracePointStdFormat(
      LoggerType&& logger
      , ContextCache& cache
      , Level level
      , const ID* ch
      , const SID* sid
      , const char* method
      , const char* file
      , int line
      , StdFormatType&& stdFormat
      , SID& subsystem
    )
    {
      (void)stdFormat;

      DispatchTracePoint(
        std::forward<LoggerType>(logger)
        , cache
        , level
        , ch
        , sid
        , method
        , file
        , line
        , subsystem
      );
    }

    template<typename LoggerType, typename StdFormatType>
    inline void DispatchTracePointStdFormat(
      LoggerType&& logger
      , ContextCache& cache
      , Level level
      , const ID* ch
      , const SID* sid
      , const char* method
      , const char* file
      , int line
      , StdFormatType&& stdFormat
      , ID& channel
      , SID& subsystem
    )
    {
      (void)stdFormat;

      DispatchTracePoint(
        std::forward<LoggerType>(logger)
        , cache
        , level
        , ch
        , sid
        , method
        , file
        , line
        , channel
        , subsystem
      );
    }

    template<typename LoggerType, typename StdFormatType>
    inline void DispatchTracePointStdFormat(
      LoggerType&& logger
      , ContextCache& cache
      , Level level
      , const ID* ch
      , const SID* sid
      , const char* method
      , const char* file
      , int line
      , StdFormatType&& stdFormat
      , ChannelPtr& channel
      , SID& subsystem
    )
    {
      (void)stdFormat;

      DispatchTracePoint(
        std::forward<LoggerType>(logger)
        , cache
        , level
        , ch
        , sid
        , method
        , file
        , line
        , channel
        , subsystem
      );
    }

    template<typename LoggerType, typename StdFormatType, typename... Args>
    inline void DispatchTracePointStdFormat(
      LoggerType&& logger
      , ContextCache& cache
      , Level level
      , const ID* ch
      , const SID* sid
      , const char* method
      , const char* file
      , int line
      , StdFormatType&& stdFormat
      , Args&&... args
    )
    {
      Context context(
        cache
        , level
        , ch
        , sid
        , method
        , file
        , line
        , MakeParams(args...)
      );

      logger->Log(context, std::forward<StdFormatType>(stdFormat), TracePointOverride(), std::forward<Args>(args)...);
    }
#endif
  }
}
