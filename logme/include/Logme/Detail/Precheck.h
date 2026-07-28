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

    template<typename T>
    using PrecheckDecay = std::remove_cv_t<std::remove_reference_t<T>>;

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

      // A following macro argument may be an explicit SID or a variadic
      // template parameter-pack expansion. Applying the channel or lexical
      // subsystem level here could reject a message whose explicit SID has a
      // more permissive override. Defer final filtering to Channel::Display.
      if (logger->HasSubsystemLevelOverrides())
        return true;

      return level >= ch->GetFilterLevel();
    }

    inline bool WouldLogArguments(
      Logger* logger
      , const Level level
      , const SID* defaultSubsystem
      , const ChannelPtr& ch
    )
    {
      (void)defaultSubsystem;
      return WouldLogWithUnknownSubsystem(logger, level, ch);
    }

    inline bool WouldLogArguments(
      Logger*
      , const Level
      , const SID*
      , Override&
    )
    {
      // Inspecting following macro arguments is unsafe because they may be
      // produced by a variadic template parameter-pack expansion. The final
      // channel and subsystem filtering is performed by Channel::Display.
      return true;
    }

    template<
      typename First
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
    )
    {
      return true;
    }

    inline bool WouldLogChannelArguments(
      Logger* logger
      , const Level level
      , const SID* defaultSubsystem
      , const ChannelPtr& ch
    )
    {
      (void)defaultSubsystem;
      return WouldLogWithUnknownSubsystem(logger, level, ch);
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

#else

  #define LOGME_PRECHECK_FIRST(...) \
    LOGME_PRECHECK_FIRST_IMPL(__VA_OPT__(__VA_ARGS__,) Logme::Detail::PrecheckEmptyArg{}, Logme::Detail::PrecheckEmptyArg{}, Logme::Detail::PrecheckEmptyArg{})

  #define LOGME_PRECHECK_FIRST_IMPL(first, ...) first


#endif

#define LOGME_WOULD_LOG_ARGS(logger, level, defaultSubsystem, ...) \
  Logme::Detail::WouldLogArguments( \
    (logger) \
    , (level) \
    , (defaultSubsystem) \
    , LOGME_PRECHECK_FIRST(__VA_ARGS__) \
  )

#define LOGME_WOULD_LOG_CHANNEL_ARGS(logger, level, defaultSubsystem, channel, ...) \
  Logme::Detail::WouldLogChannelArguments( \
    (logger) \
    , (level) \
    , (defaultSubsystem) \
    , (channel) \
  )
