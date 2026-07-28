#pragma once

#include <cstddef>
#include <tuple>
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

    template<typename Types, typename F>
    class PrecheckLazyArg
    {
    public:
      using ArgumentTypes = Types;

      explicit PrecheckLazyArg(F function)
        : Function(std::move(function))
      {
      }

      template<typename Callback>
      decltype(auto) WithValues(Callback&& callback) const
      {
        return Function(std::forward<Callback>(callback));
      }

    private:
      F Function;
    };

    template<typename Types, typename F>
    inline auto MakePrecheckLazyArg(std::type_identity<Types>, F&& function)
    {
      return PrecheckLazyArg<Types, std::decay_t<F>>(std::forward<F>(function));
    }

    template<typename T>
    using PrecheckDecay = std::remove_cv_t<std::remove_reference_t<T>>;

    template<typename Types>
    struct PrecheckArgTypesInfo;

    template<>
    struct PrecheckArgTypesInfo<std::tuple<>>
    {
      static constexpr std::size_t Count = 0;
      using First = PrecheckEmptyArg;
    };

    template<typename FirstArg, typename... Rest>
    struct PrecheckArgTypesInfo<std::tuple<FirstArg, Rest...>>
    {
      static constexpr std::size_t Count = 1 + sizeof...(Rest);
      using First = PrecheckDecay<FirstArg>;
    };

    template<typename LazyArg>
    using PrecheckLazyInfo = PrecheckArgTypesInfo<typename LazyArg::ArgumentTypes>;

    template<typename LazyArg>
    inline constexpr bool PrecheckLazyIsSingleSID =
         PrecheckLazyInfo<LazyArg>::Count == 1
      && std::is_same_v<typename PrecheckLazyInfo<LazyArg>::First, SID>;

    template<typename LazyArg>
    inline constexpr bool PrecheckLazyStartsWithSID =
         PrecheckLazyInfo<LazyArg>::Count != 0
      && std::is_same_v<typename PrecheckLazyInfo<LazyArg>::First, SID>;

    template<typename LazyArg>
    inline constexpr bool PrecheckLazyIsSingleChannel =
         PrecheckLazyInfo<LazyArg>::Count == 1
      && std::is_same_v<typename PrecheckLazyInfo<LazyArg>::First, ChannelPtr>;

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

    inline bool WouldLogWithUnknownSubsystem(
      Logger* logger
      , const Level level
      , const ChannelPtr& ch
    )
    {
      if (level >= Level::LEVEL_ERROR && logger->GetErrorChannel() != nullptr)
        return true;

      if (!ch || !ch->GetActive())
        return false;

      if (logger->HasSubsystemLevelOverrides())
        return true;

      return level >= ch->GetFilterLevel();
    }

    template<typename SecondLazy, typename ThirdLazy>
    inline bool WouldLogArguments(
      Logger* logger
      , const Level level
      , const SID* defaultSubsystem
      , const ChannelPtr& ch
      , const SecondLazy& second
      , const ThirdLazy&
    )
    {
      if constexpr (PrecheckLazyIsSingleSID<SecondLazy>)
      {
        return second.WithValues([&](const SID& sid)
        {
          return WouldLog(logger, level, defaultSubsystem, ch, &sid);
        });
      }
      else if constexpr (PrecheckLazyStartsWithSID<SecondLazy>)
      {
        // A forwarded parameter pack starts with an explicit SID. Its value
        // cannot be read without also evaluating all following format values.
        return WouldLogWithUnknownSubsystem(logger, level, ch);
      }
      else
      {
        // The first non-channel argument is the format string. Everything
        // after it is a formatting argument and must not participate in
        // precheck parsing.
        return WouldLog(logger, level, defaultSubsystem, ch);
      }
    }

    template<typename SecondLazy, typename ThirdLazy>
    inline bool WouldLogArguments(
      Logger* logger
      , const Level level
      , const SID* defaultSubsystem
      , Override&
      , const SecondLazy& second
      , const ThirdLazy& third
    )
    {
      if constexpr (PrecheckLazyIsSingleChannel<SecondLazy>)
      {
        return second.WithValues([&](const ChannelPtr& ch)
        {
          if constexpr (PrecheckLazyIsSingleSID<ThirdLazy>)
          {
            return third.WithValues([&](const SID& sid)
            {
              return WouldLog(logger, level, defaultSubsystem, ch, &sid);
            });
          }
          else if constexpr (PrecheckLazyStartsWithSID<ThirdLazy>)
          {
            return WouldLogWithUnknownSubsystem(logger, level, ch);
          }
          else
          {
            return WouldLog(logger, level, defaultSubsystem, ch);
          }
        });
      }
      else
      {
        // A forwarded parameter pack may contain the channel and the rest of
        // the call. Evaluating it here would defeat precheck, so defer.
        return true;
      }
    }

    template<
      typename First
      , typename SecondLazy
      , typename ThirdLazy
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
      , const ThirdLazy&
    )
    {
      // The first argument is the format string or an ID handled by the
      // normal dispatcher. No later argument may be interpreted as a SID.
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
      if constexpr (PrecheckLazyIsSingleSID<FirstLazy>)
      {
        return first.WithValues([&](const SID& sid)
        {
          return WouldLog(logger, level, defaultSubsystem, ch, &sid);
        });
      }
      else if constexpr (PrecheckLazyStartsWithSID<FirstLazy>)
      {
        return WouldLogWithUnknownSubsystem(logger, level, ch);
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
  #define LOGME_PRECHECK_THIRD(...) Logme::Detail::PrecheckEmptyArg{}

#else

  #define LOGME_PRECHECK_FIRST(...) \
    LOGME_PRECHECK_FIRST_IMPL(__VA_OPT__(__VA_ARGS__,) Logme::Detail::PrecheckEmptyArg{}, Logme::Detail::PrecheckEmptyArg{}, Logme::Detail::PrecheckEmptyArg{})

  #define LOGME_PRECHECK_FIRST_IMPL(first, ...) first

  #define LOGME_PRECHECK_SECOND(...) \
    LOGME_PRECHECK_SECOND_IMPL(__VA_OPT__(__VA_ARGS__,) Logme::Detail::PrecheckEmptyArg{}, Logme::Detail::PrecheckEmptyArg{}, Logme::Detail::PrecheckEmptyArg{})

  #define LOGME_PRECHECK_SECOND_IMPL(first, second, ...) second

  #define LOGME_PRECHECK_THIRD(...) \
    LOGME_PRECHECK_THIRD_IMPL(__VA_OPT__(__VA_ARGS__,) Logme::Detail::PrecheckEmptyArg{}, Logme::Detail::PrecheckEmptyArg{}, Logme::Detail::PrecheckEmptyArg{})

  #define LOGME_PRECHECK_THIRD_IMPL(first, second, third, ...) third

#endif

#define LOGME_PRECHECK_TYPE_LIST(expression) \
  decltype(std::forward_as_tuple(expression))

#define LOGME_PRECHECK_LAZY(expression) \
  Logme::Detail::MakePrecheckLazyArg( \
    std::type_identity<LOGME_PRECHECK_TYPE_LIST(expression)>{} \
    , [&](auto&& callback) -> decltype(auto) \
    { \
      return std::forward<decltype(callback)>(callback)(expression); \
    } \
  )

#define LOGME_WOULD_LOG_ARGS(logger, level, defaultSubsystem, ...) \
  Logme::Detail::WouldLogArguments( \
    (logger) \
    , (level) \
    , (defaultSubsystem) \
    , LOGME_PRECHECK_FIRST(__VA_ARGS__) \
    , LOGME_PRECHECK_LAZY(LOGME_PRECHECK_SECOND(__VA_ARGS__)) \
    , LOGME_PRECHECK_LAZY(LOGME_PRECHECK_THIRD(__VA_ARGS__)) \
  )

#define LOGME_WOULD_LOG_CHANNEL_ARGS(logger, level, defaultSubsystem, channel, ...) \
  Logme::Detail::WouldLogChannelArguments( \
    (logger) \
    , (level) \
    , (defaultSubsystem) \
    , (channel) \
    , LOGME_PRECHECK_LAZY(LOGME_PRECHECK_FIRST(__VA_ARGS__)) \
  )
