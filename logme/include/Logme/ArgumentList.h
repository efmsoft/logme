#pragma once

#include <iostream>
#include <sstream>
#include <string>

#include <Logme/Printer.h>
#include <Logme/Types.h>

namespace Logme
{
  /// <summary>
  /// Formats a named argument pair as "name=value". Values are formatted through FormatValue().
  /// </summary>
  template<typename T>
  std::string FormatOne(const std::pair<const char*, T>& item)
  {
    return std::string(item.first) + "=" + FormatValue(item.second);
  }

  /// <summary>
  /// Formats a single unnamed value through FormatValue().
  /// </summary>
  template<typename T>
  std::string FormatOne(const T& value)
  {
    return FormatValue(value);
  }

  /// <summary>
  /// Formats tuple elements as a comma-separated list. Named elements appear as "name=value".
  /// </summary>
  template<typename Tuple, std::size_t... Is>
  std::string FormatTupleWithCommas(const Tuple& tup, std::index_sequence<Is...>)
  {
    std::ostringstream oss;
    ((oss << FormatOne(std::get<Is>(tup)) << (Is + 1 < sizeof...(Is) ? ", " : "")), ...);

    return oss.str();
  }

  /// <summary>
  /// Formats all arguments into a comma-separated string for procedure-print macros.
  /// Arguments wrapped with NAMED() are printed as "name=value".
  /// </summary>
  template<typename... Args>
  std::string FormatAll(Args&&... args)
  {
    auto tup = std::make_tuple(std::forward<Args>(args)...);
    return FormatTupleWithCommas(tup, std::index_sequence_for<Args...>{});
  }
}

/// <summary>
/// Wraps a variable or expression so ArgumentList formatting prints its source text and value as
/// "name=value". Intended for ARGS() and the ARGS1..ARGS5 helpers.
/// </summary>
#define NAMED(x) std::make_pair(#x, (x))

/// <summary>
/// Formats arguments for procedure logging. Unwrapped arguments are printed as values; arguments
/// wrapped with NAMED() are printed as "name=value". The returned pointer is intended to be consumed
/// immediately by a Logme procedure macro in the same full expression.
/// </summary>
#define ARGS(...) Logme::FormatAll(__VA_ARGS__).c_str()

/// <summary>
/// Formats one procedure argument as "arg1=value" for LogmeP/LogmePV-style macros.
/// </summary>
#define ARGS1(arg1) \
  Logme::FormatAll(NAMED(arg1)).c_str()

/// <summary>
/// Formats two procedure arguments as "arg=value" pairs separated by comma.
/// </summary>
#define ARGS2(arg1, arg2) \
  Logme::FormatAll(NAMED(arg1), NAMED(arg2)).c_str()

/// <summary>
/// Formats three procedure arguments as "arg=value" pairs separated by comma.
/// </summary>
#define ARGS3(arg1, arg2, arg3) \
  Logme::FormatAll(NAMED(arg1), NAMED(arg2), NAMED(arg3)).c_str()

/// <summary>
/// Formats four procedure arguments as "arg=value" pairs separated by comma.
/// </summary>
#define ARGS4(arg1, arg2, arg3, arg4) \
  Logme::FormatAll(NAMED(arg1), NAMED(arg2), NAMED(arg3), NAMED(arg4)).c_str()

/// <summary>
/// Formats five procedure arguments as "arg=value" pairs separated by comma.
/// </summary>
#define ARGS5(arg1, arg2, arg3, arg4, arg5) \
  Logme::FormatAll(NAMED(arg1), NAMED(arg2), NAMED(arg3), NAMED(arg4), NAMED(arg5)).c_str()

