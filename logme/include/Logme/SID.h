#pragma once

#include <memory>
#include <string>

#include <Logme/Types.h>

namespace Logme
{
  /// <summary>
  /// Compact subsystem identifier printed by log output when the Subsystem output flag is enabled.
  /// Text subsystem names are packed into the 64-bit Name field and are intended to be short.
  /// </summary>
  struct SID
  {
    /// <summary>Packed subsystem value.</summary>
    uint64_t Name;

    /// <summary>Builds a subsystem identifier from an already packed 64-bit value.</summary>
    LOGMELNK static SID Build(uint64_t name);
    /// <summary>Builds a subsystem identifier from a short string. Up to eight characters are packed.</summary>
    LOGMELNK static SID Build(const char* name_upto8chars);
    /// <summary>Builds a subsystem identifier from a short string. Up to eight characters are packed.</summary>
    LOGMELNK static SID Build(const std::string& name_upto8chars);
  };

  typedef std::shared_ptr<SID> SIDPtr;
}

/// <summary>
/// Default subsystem identifier used by Logme macros when no subsystem is declared explicitly.
/// </summary>
static constexpr const Logme::SID SUBSID{0};

/// <summary>
/// Declares a subsystem identifier variable for use by Logme macros. Use it as SUBSID in a scope or
/// class where log messages should carry a subsystem tag when OutputFlags::Subsystem is enabled.
/// String names are packed by Logme::SID::Build(), using up to eight characters.
/// </summary>
#define LOGME_SUBSYSTEM(var, name) \
  Logme::SID var = Logme::SID::Build(name)
