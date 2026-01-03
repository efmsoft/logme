#include <Logme/Printer.h>
#include <Logme/Types.h>

#include <iomanip>
#include <sstream>
#include <stdint.h>

using namespace Logme;

Printer Logme::None;
template<> LOGMELNK std::string FormatValue(const Printer& v) { return std::string(); }

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

template<> std::string StdFormat<int8_t>(const int8_t& v)
{
  std::stringstream ss;
  ss << (int)v;
  return ss.str();
}

template<> std::string StdFormat<uint8_t>(const uint8_t& v)
{
  std::stringstream ss;
  ss << (unsigned)v;
  return ss.str();
}

template<> std::string StdXFormat<int8_t>(const int8_t& v)
{
  std::stringstream ss;
  ss << "0x" << std::hex << (unsigned)(uint8_t)v;
  return ss.str();
}

template<> std::string StdXFormat<uint8_t>(const uint8_t& v)
{
  std::stringstream ss;
  ss << "0x" << std::hex << (unsigned)v;
  return ss.str();
}

#define GENERATE_TYPE_FORMATTER(T) \
  template<> LOGMELNK std::string FormatValue(const T& v) { return StdFormat<T>((T)v); } \
  template<> LOGMELNK std::string FormatValue(const x##T& v) { return StdXFormat<T>((T)v); } \
  template<> LOGMELNK std::string FormatValue(const u##T& v) { return StdFormat<u##T>((u##T)v); } \
  template<> LOGMELNK std::string FormatValue(const xu##T& v) { return StdXFormat<u##T>((u##T)v); }

#define GENERATE_TYPE_FORMATTER0(T) \
  template<> LOGMELNK std::string FormatValue(const T& v) { return StdFormat<T>((T)v); } \
  template<> LOGMELNK std::string FormatValue(const unsigned T& v) { return StdFormat<unsigned T>((unsigned T)v); }

/////////////////////////////////////

GENERATE_TYPE_FORMATTER(int8_t)
GENERATE_TYPE_FORMATTER(int16_t)
GENERATE_TYPE_FORMATTER(int32_t)
GENERATE_TYPE_FORMATTER(int64_t)

template<> LOGMELNK std::string FormatValue(const std::string& v) { return v; }
template<> LOGMELNK std::string FormatValue(char const* const& v) { return v; }
template<> LOGMELNK std::string FormatValue(char* const& v) { return v; }
template<> LOGMELNK std::string FormatValue(bool const& v) { return v ? "true" : "false"; }
