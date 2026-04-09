#pragma once

namespace Logme
{
  namespace Detail
  {
    inline Context::Params MakeParams()
    {
      return Context::Params();
    }

    template<typename First, typename... Rest>
    inline Context::Params MakeParams(const First&, Rest&&...)
    {
      return Context::Params();
    }

    template<typename... Rest>
    inline Context::Params MakeParams(const ID& ch, Rest&&...)
    {
      return Context::Params(ch);
    }

    template<typename... Rest>
    inline Context::Params MakeParams(const SID& sid, Rest&&...)
    {
      return Context::Params(sid);
    }

    template<typename... Rest>
    inline Context::Params MakeParams(
      const ID& ch
      , const SID& sid
      , Rest&&...
    )
    {
      return Context::Params(ch, sid);
    }

    template<typename LoggerType, typename... Args>
    inline decltype(auto) Dispatch(
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

      return logger->Log(context, std::forward<Args>(args)...);
    }

    template<typename LoggerType, typename StdFormatType, typename... Args>
    inline void DispatchStdFormat(
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

      logger->Log(context, std::forward<StdFormatType>(stdFormat), std::forward<Args>(args)...);
    }
  }
}
