#include <Logme/Printer.h>
#include <Logme/Types.h>

#include <iomanip>
#include <sstream>
#include <stdint.h>

using namespace Logme;

Printer Logme::None;
 
template<typename T> std::string StdFormat(const T& v)
{
  std::stringstream ss;
  ss << v;
  return ss.str();
}

template<typename T> std::string StdXFormat(const T& v)
{
  std::stringstream ss;
  ss << "0x" << std::hex << v;
  return ss.str();
}

#define GENERATE_TYPE_FORMATTER(T) \
  std::string FormatValue(const T& v) { return StdFormat<T>((T)v); } \
  std::string FormatValue(const x##T& v) { return StdXFormat<T>((T)v); } \
  std::string FormatValue(const u##T& v) { return StdFormat<u##T>((u##T)v); } \
  std::string FormatValue(const xu##T& v) { return StdXFormat<u##T>((u##T)v); }

#define GENERATE_TYPE_FORMATTER0(T) \
  std::string FormatValue(const T& v) { return StdFormat<T>((T)v); } \
  std::string FormatValue(const unsigned T& v) { return StdFormat<unsigned T>((unsigned T)v); }

/////////////////////////////////////

GENERATE_TYPE_FORMATTER(int8_t)
GENERATE_TYPE_FORMATTER(int16_t)
GENERATE_TYPE_FORMATTER(int32_t)
GENERATE_TYPE_FORMATTER(int64_t)

#if  !defined(__GNUC__) || defined(__clang__)
GENERATE_TYPE_FORMATTER0(long)
#endif

std::string FormatValue(const std::string& v) { return v; }
std::string FormatValue(char const* const& v) { return v; }
std::string FormatValue(char* const& v) { return v; }

namespace Logme
{
  GENERATE_TYPE_FORMATTER(int8_t)
  GENERATE_TYPE_FORMATTER(int16_t)
  GENERATE_TYPE_FORMATTER(int32_t)
  GENERATE_TYPE_FORMATTER(int64_t)
#if  !defined(__GNUC__) || defined(__clang__)
  GENERATE_TYPE_FORMATTER0(long)
#endif

  std::string FormatValue(const std::string& v) { return v; }
  std::string FormatValue(char const* const& v) { return v; }
  std::string FormatValue(char* const& v) { return v; }
}
