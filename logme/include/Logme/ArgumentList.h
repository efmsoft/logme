#pragma once

#include <iostream>
#include <sstream>
#include <string>

#include <Logme/Printer.h>
#include <Logme/Types.h>

namespace Logme
{
  template<typename T>
  std::string FormatOne(const std::pair<const char*, T>& item)
  {
    return std::string(item.first) + "=" + FormatValue(item.second);
  }

  template<typename T>
  std::string FormatOne(const T& value)
  {
    return FormatValue(value);
  }

  template<typename Tuple, std::size_t... Is>
  std::string FormatTupleWithCommas(const Tuple& tup, std::index_sequence<Is...>)
  {
    std::ostringstream oss;
    ((oss << FormatOne(std::get<Is>(tup)) << (Is + 1 < sizeof...(Is) ? ", " : "")), ...);

    return oss.str();
  }

  template<typename... Args>
  std::string FormatAll(Args&&... args)
  {
    auto tup = std::make_tuple(std::forward<Args>(args)...);
    return FormatTupleWithCommas(tup, std::index_sequence_for<Args...>{});
  }
}

#define NAMED(x) std::make_pair(#x, (x))

#define ARGS(...) Logme::FormatAll(__VA_ARGS__).c_str()

#define ARGS1(arg1) \
  Logme::FormatAll(NAMED(arg1)).c_str()

#define ARGS2(arg1, arg2) \
  Logme::FormatAll(NAMED(arg1), NAMED(arg2)).c_str()

#define ARGS3(arg1, arg2, arg3) \
  Logme::FormatAll(NAMED(arg1), NAMED(arg2), NAMED(arg3)).c_str()

#define ARGS4(arg1, arg2, arg3, arg4) \
  Logme::FormatAll(NAMED(arg1), NAMED(arg2), NAMED(arg3), NAMED(arg4)).c_str()

#define ARGS5(arg1, arg2, arg3, arg4, arg5) \
  Logme::FormatAll(NAMED(arg1), NAMED(arg2), NAMED(arg3), NAMED(arg4), NAMED(arg5)).c_str()

