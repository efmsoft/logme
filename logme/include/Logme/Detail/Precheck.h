#pragma once

#include <type_traits>
#include <utility>

#include <Logme/Logger.h>

namespace Logme
{
  namespace Detail
  {
    // Early disabled-path filtering and null-sink equivalent.
    // A channel with no effective output destinations can be rejected before
    // argument preparation, formatting, and backend dispatch are performed.
    struct PrecheckEmptyArg
    {
    };

    template<typename ResultType, typename F>
    class PrecheckLazyArg
    {
    public:
      using Result = ResultType;

      explicit PrecheckLazyArg(F function)
        : Function(std::move(function))
      {
      }

      template<typename Callback>
      decltype(auto) WithValue(Callback&& callback) const
      {
        return Function(std::forward<Callback>(callback));
      }

    private:
      F Function;
    };

    template<typename ResultType, typename F>
    inline auto MakePrecheckLazyArg(F&& function)
    {
      return PrecheckLazyArg<ResultType, std::decay_t<F>>(std::forward<F>(function));
    }

    template<typename T>
    using PrecheckDecay = std::remove_cv_t<std::remove_reference_t<T>>;

    template<typename LazyArg>
    using PrecheckLazyResult = PrecheckDecay<typename LazyArg::Result>;

    inline bool WouldLog(
      Logger* logger
      , const Level level
      , const SID* defaultSubsystem
      , const ChannelPtr& ch
      , const SID* explicitSubsystem = nullptr
    )
    {
      if (level >= Level::LEVEL_ERROR && logger->GetErrorChannel() != nullptr)
        return true;

      if (!ch)
        return false;

      if (explicitSubsystem != nullptr)
      {
        Level subsystemLevel;
        if (logger->GetSubsystemLevel(*explicitSubsystem, subsystemLevel))
          return level >= subsystemLevel && ch->GetActive();
      }
      else if (logger->HasSubsystemLevelOverrides())
      {
        if (logger->IsSubsystemDefinedForCurrentThread())
          return true;

        if (defaultSubsystem != nullptr && defaultSubsystem->Name != 0)
        {
          Level subsystemLevel;
          if (logger->GetSubsystemLevel(*defaultSubsystem, subsystemLevel))
            return level >= subsystemLevel && ch->GetActive();
        }
      }

      if (level < ch->GetFilterLevel())
        return false;

      return ch->GetActive();
    }

    template<typename SecondLazy>
    inline bool WouldLogArguments(
      Logger* logger
      , const Level level
      , const SID* defaultSubsystem
      , const ChannelPtr& ch
      , const SecondLazy& second
    )
    {
      using SecondType = PrecheckLazyResult<SecondLazy>;

      if constexpr (std::is_same_v<SecondType, SID>)
      {
        return second.WithValue([&](const SID& sid)
        {
          return WouldLog(logger, level, defaultSubsystem, ch, &sid);
        });
      }
      else
      {
        return WouldLog(logger, level, defaultSubsystem, ch);
      }
    }

    template<typename SecondLazy>
    inline bool WouldLogArguments(
      Logger* logger
      , const Level level
      , const SID* defaultSubsystem
      , Override&
      , const SecondLazy& second
    )
    {
      using SecondType = PrecheckLazyResult<SecondLazy>;

      if constexpr (std::is_same_v<SecondType, ChannelPtr>)
      {
        return second.WithValue([&](const ChannelPtr& ch)
        {
          // The following argument may be a parameter-pack expansion. Do not
          // inspect it in the preprocessor; final SID filtering is performed
          // by Channel::Display.
          return WouldLog(logger, level, defaultSubsystem, ch);
        });
      }
      else
      {
        return true;
      }
    }

    template<
      typename First
      , typename SecondLazy
      , std::enable_if_t<
           !std::is_same_v<PrecheckDecay<First>, ChannelPtr>
        && !std::is_same_v<PrecheckDecay<First>, Override>
        , int
      > = 0
    >
    inline bool WouldLogArguments(
      Logger*
      , const Level
      , const SID*
      , First&&
      , const SecondLazy&
    )
    {
      return true;
    }

    template<typename FirstLazy>
    inline bool WouldLogChannelArguments(
      Logger* logger
      , const Level level
      , const SID* defaultSubsystem
      , const ChannelPtr& ch
      , const FirstLazy& first
    )
    {
      using FirstType = PrecheckLazyResult<FirstLazy>;

      if constexpr (std::is_same_v<FirstType, SID>)
      {
        return first.WithValue([&](const SID& sid)
        {
          return WouldLog(logger, level, defaultSubsystem, ch, &sid);
        });
      }
      else
      {
        return WouldLog(logger, level, defaultSubsystem, ch);
      }
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

#if defined(__INTELLISENSE__) || (defined(_MSC_VER) && defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL && !(defined(__CLION_IDE__) || defined(__JETBRAINS_IDE__)))

  #define LOGME_PRECHECK_FIRST(...) Logme::Detail::PrecheckEmptyArg{}
  #define LOGME_PRECHECK_SECOND(...) Logme::Detail::PrecheckEmptyArg{}

#else

  #define LOGME_PRECHECK_FIRST(...) \
    LOGME_PRECHECK_FIRST_IMPL(__VA_OPT__(__VA_ARGS__,) Logme::Detail::PrecheckEmptyArg{}, Logme::Detail::PrecheckEmptyArg{}, Logme::Detail::PrecheckEmptyArg{})

  #define LOGME_PRECHECK_FIRST_IMPL(first, ...) first

  #define LOGME_PRECHECK_SECOND(...) \
    LOGME_PRECHECK_SECOND_IMPL(__VA_OPT__(__VA_ARGS__,) Logme::Detail::PrecheckEmptyArg{}, Logme::Detail::PrecheckEmptyArg{}, Logme::Detail::PrecheckEmptyArg{})

  #define LOGME_PRECHECK_SECOND_IMPL(first, second, ...) second


#endif

#define LOGME_PRECHECK_LAZY(expression) \
  Logme::Detail::MakePrecheckLazyArg<decltype((expression))>( \
    [&](auto&& callback) -> decltype(auto) \
    { \
      return std::forward<decltype(callback)>(callback)((expression)); \
    } \
  )

#define LOGME_WOULD_LOG_ARGS(logger, level, defaultSubsystem, ...) \
  Logme::Detail::WouldLogArguments( \
    (logger) \
    , (level) \
    , (defaultSubsystem) \
    , LOGME_PRECHECK_FIRST(__VA_ARGS__) \
    , LOGME_PRECHECK_LAZY(LOGME_PRECHECK_SECOND(__VA_ARGS__)) \
  )

#define LOGME_WOULD_LOG_CHANNEL_ARGS(logger, level, defaultSubsystem, channel, ...) \
  Logme::Detail::WouldLogChannelArguments( \
    (logger) \
    , (level) \
    , (defaultSubsystem) \
    , (channel) \
    , LOGME_PRECHECK_LAZY(LOGME_PRECHECK_FIRST(__VA_ARGS__)) \
  )
