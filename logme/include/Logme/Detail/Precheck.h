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

    inline const ChannelPtr& ResolveDoChannel(Logger*, const ChannelPtr& ch)
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

#elif defined(_MSC_VER) && defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL && !(defined(__CLION_IDE__) || defined(__JETBRAINS_IDE__))

  #define LOGME_FIRST_OR_EMPTY(...) Logme::Detail::PrecheckEmptyArg{}

#else

  #define LOGME_FIRST_OR_EMPTY(...) \
    LOGME_FIRST_OR_EMPTY_IMPL(__VA_OPT__(__VA_ARGS__,) Logme::Detail::PrecheckEmptyArg{})

  #define LOGME_FIRST_OR_EMPTY_IMPL(first, ...) \
    first

#endif

#define LOGME_WOULD_LOG_FIRST(logger, level, ...) \
  Logme::Detail::WouldLogFirst((logger), (level), LOGME_FIRST_OR_EMPTY(__VA_ARGS__))
