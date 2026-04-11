#pragma once

namespace Logme
{
  namespace Detail
  {
    struct PrecheckEmptyArg
    {
    };

    inline bool WouldLog(Logger* logger, Level level, const ChannelPtr& ch)
    {
      if (level >= Level::LEVEL_ERROR && logger->GetErrorChannel() != nullptr)
        return true;

      if (!ch)
        return false;

      if (level < ch->GetFilterLevel())
        return false;

      return ch->GetActive();
    }

    inline bool WouldLogFirst(Logger*, Level, const PrecheckEmptyArg&)
    {
      return true;
    }

    inline bool WouldLogFirst(Logger* logger, Level level, const ChannelPtr& ch)
    {
      return WouldLog(logger, level, ch);
    }

    template<typename T>
    inline bool WouldLogFirst(Logger*, Level, const T&)
    {
      return true;
    }

    inline ChannelPtr ResolveDoChannel(Logger*, const ChannelPtr& ch)
    {
      return ch;
    }

    inline ChannelPtr ResolveDoChannel(Logger* logger, const ID& ch)
    {
      return logger->GetExistingChannel(ch);
    }
  }
}

#if defined(__INTELLISENSE__)

  #define LOGME_FIRST_OR_EMPTY_MSVC(_0, first, ...) first
  #define LOGME_FIRST_OR_EMPTY(...) \
    LOGME_FIRST_OR_EMPTY_MSVC(_, ## __VA_ARGS__, Logme::Detail::PrecheckEmptyArg{})

#elif defined(_MSC_VER) && !(defined(__CLION_IDE__) || defined(__JETBRAINS_IDE__))

  // current working MSVC build implementation
  #define LOGME_EXPAND(x) x

  #define LOGME_FIRST_OR_EMPTY_MSVC_EMPTY(...) \
    Logme::Detail::PrecheckEmptyArg{}

  #define LOGME_FIRST_OR_EMPTY_MSVC_ARGS(first, ...) \
    first

  #define LOGME_FIRST_OR_EMPTY_MSVC_SELECT( \
    _0 \
    , _1, _2, _3, _4, _5, _6, _7, _8, _9 \
    , _10, _11, _12, _13, _14, _15, _16, _17, _18, _19 \
    , _20, _21, _22, _23, _24, _25, _26, _27, _28, _29 \
    , _30, _31, _32 \
    , name \
    , ... \
  ) \
    name

  #define LOGME_FIRST_OR_EMPTY(...) \
    LOGME_EXPAND( \
      LOGME_FIRST_OR_EMPTY_MSVC_SELECT( \
        _ \
        , ## __VA_ARGS__ \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_ARGS \
        , LOGME_FIRST_OR_EMPTY_MSVC_EMPTY \
      )(__VA_ARGS__) \
    )

#else

  #define LOGME_FIRST_OR_EMPTY(...) \
    LOGME_FIRST_OR_EMPTY_IMPL(__VA_OPT__(__VA_ARGS__,) Logme::Detail::PrecheckEmptyArg{})

  #define LOGME_FIRST_OR_EMPTY_IMPL(first, ...) \
    first

#endif

#define LOGME_WOULD_LOG_FIRST(logger, level, ...) \
  Logme::Detail::WouldLogFirst((logger), (level), LOGME_FIRST_OR_EMPTY(__VA_ARGS__))
