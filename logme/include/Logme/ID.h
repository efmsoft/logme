#pragma once

#include <memory>

namespace Logme
{
  /// <summary>
  /// Lightweight channel identifier used by logging macros and channel creation APIs.
  /// It stores a pointer to a channel name and does not own the pointed string.
  /// Use SafeID when the name must be stored safely beyond the lifetime of the original text.
  /// </summary>
  struct ID
  {
    /// <summary>Channel name pointer. The ID object does not own this string.</summary>
    const char* Name;
  };
}

/// <summary>
/// Default channel identifier used by Logme macros when no channel is passed explicitly.
/// </summary>
static constexpr const Logme::ID CH{""};
/// <summary>
/// Internal logme channel identifier. Library code uses it for logme's own diagnostic messages.
/// </summary>
static constexpr const Logme::ID CHINT{"logme"};

/// <summary>
/// Declares a channel identifier variable for use with Logme macros and CreateChannel().
/// The created Logme::ID stores the supplied name pointer and does not copy the string.
/// </summary>
#define LOGME_CHANNEL(var, name) \
  Logme::ID var{name}
