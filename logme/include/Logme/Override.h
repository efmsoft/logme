#pragma once

#include <memory>
#include <unordered_map>

#include <Logme/OutputFlags.h>
#include <Logme/Types.h>

namespace Logme
{
  /// <summary>
  /// Text replacement rule used by an override to shorten method names before they are printed.
  /// </summary>
  struct ShortenerPair
  {
    const char* SerachFor;
    const char* ReplaceOn;
  };

  /// <summary>
  /// Per-message logging override. It can add or remove output flags, limit the number of times a
  /// message is printed, rate-limit repeated messages, or provide method-name shortening rules.
  /// Pass an Override object to LogmeX macros as one of the optional arguments.
  /// </summary>
  struct Override
  {
    /// <summary>Flags temporarily added for this log record.</summary>
    OutputFlags Add;
    /// <summary>Flags temporarily removed for this log record.</summary>
    OutputFlags Remove;

    /// <summary>Maximum number of times this override may allow a matching message; -1 means unlimited.</summary>
    int MaxRepetitions;
    /// <summary>Number of times this override has already been used.</summary>
    int Repetitions;

    /// <summary>Minimum delay between matching messages, in milliseconds; 0 disables rate limiting.</summary>
    uint64_t MaxFrequency;
    /// <summary>Timestamp of the last message allowed by the frequency limiter.</summary>
    uint64_t LastTime;

    /// <summary>Optional method-name shortening table used while applying this override.</summary>
    ShortenerPair* Shortener;

    /// <summary>
    /// Creates a message override.
    /// </summary>
    /// <param name="reps">Maximum number of matching messages to print; -1 means unlimited.</param>
    /// <param name="noMoreThanOnceEveryXMillisec">Minimum delay between matching messages in milliseconds.</param>
    LOGMELNK Override(
      int reps = -1
      , uint64_t noMoreThanOnceEveryXMillisec = 0
    );
  };

  typedef std::shared_ptr<Override> OverridePtr;

  /// <summary>
  /// Stores one-shot overrides keyed by call site. Derive from this class when an object needs
  /// LOGME_ONCE4THIS-style logging: the same source line logs only once for that object instance.
  /// </summary>
  struct OneTimeOverrideGenerator
  {
    std::unordered_map<uint64_t, Override> Overrides;

    LOGMELNK OneTimeOverrideGenerator();

    /// <summary>
    /// Returns the persistent one-shot override associated with a function/line pair.
    /// </summary>
    /// <param name="func">Function name, normally __func__ from LOGME_ONCE4THIS or LOGME_ONCE4CALL.</param>
    /// <param name="line">Source line, normally __LINE__ from LOGME_ONCE4THIS or LOGME_ONCE4CALL.</param>
    LOGMELNK virtual Override& GetOneTimeOverride(
      const char* func
      , int line
    );
  };

  /// <summary>
  /// Stores rate-limit overrides keyed by call site. Derive from this class when an object needs
  /// LOGME_EVERY4THIS-style logging: the same source line logs no more often than the requested interval.
  /// </summary>
  struct EveryTimeOverrideGenerator
  {
    std::unordered_map<uint64_t, Override> Overrides;

    LOGMELNK EveryTimeOverrideGenerator();

    /// <summary>
    /// Returns the persistent rate-limit override associated with a function/line pair.
    /// </summary>
    /// <param name="func">Function name, normally __func__ from LOGME_EVERY4THIS or LOGME_EVERY4CALL.</param>
    /// <param name="line">Source line, normally __LINE__ from LOGME_EVERY4THIS or LOGME_EVERY4CALL.</param>
    /// <param name="ms">Minimum delay between printed messages from this call site, in milliseconds.</param>
    LOGMELNK virtual Override& GetEveryOverride(
      const char* func
      , int line
      , uint64_t ms
    );
  };

  /// <summary>
  /// Combines one-shot and rate-limit override storage for a local function call scope.
  /// LOGME_CALL_SCOPE creates an object of this type.
  /// </summary>
  struct OverrideGenerator 
    : public OneTimeOverrideGenerator
    , public EveryTimeOverrideGenerator
  {
  };
}

/// <summary>
/// Returns a one-shot override owned by the current object. The class must derive from
/// Logme::OneTimeOverrideGenerator or Logme::OverrideGenerator. Use it as an optional argument to
/// a LogmeX macro when a message from the same source line should be printed only once per object.
/// </summary>
#define LOGME_ONCE4THIS GetOneTimeOverride(__func__, __LINE__)

/// <summary>
/// Returns a rate-limit override owned by the current object. The class must derive from
/// Logme::EveryTimeOverrideGenerator or Logme::OverrideGenerator. Use it as an optional argument to
/// a LogmeX macro when a message from the same source line should be printed no more often than ms.
/// </summary>
#define LOGME_EVERY4THIS(ms) GetEveryOverride(__func__, __LINE__, ms)

/// <summary>
/// Declares local override storage for the current function invocation. Put this macro at the start
/// of a function when LOGME_ONCE4CALL or LOGME_EVERY4CALL is used later in the same scope.
/// Each invocation gets its own storage, so "once" means once during this call, not once globally.
/// </summary>
#define LOGME_CALL_SCOPE \
  Logme::OverrideGenerator please_define_macro_LOGME_CALL_SCOPE

/// <summary>
/// Returns a one-shot override stored in the LOGME_CALL_SCOPE object. Use it as an optional argument
/// to a LogmeX macro when a message from the same source line should be printed only once during the
/// current function invocation. LOGME_CALL_SCOPE must be declared in the same scope first.
/// </summary>
#define LOGME_ONCE4CALL \
  please_define_macro_LOGME_CALL_SCOPE.GetOneTimeOverride(__func__, __LINE__)

/// <summary>
/// Returns a rate-limit override stored in the LOGME_CALL_SCOPE object. Use it as an optional argument
/// to a LogmeX macro when a message from the same source line should be printed no more often than ms
/// during the current function invocation. LOGME_CALL_SCOPE must be declared in the same scope first.
/// </summary>
#define LOGME_EVERY4CALL(ms) \
  please_define_macro_LOGME_CALL_SCOPE.GetEveryOverride(__func__, __LINE__, ms)
