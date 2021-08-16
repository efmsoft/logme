#include <Logme/Printer.h>
#include <stdint.h>

using namespace Logme;

Printer Logme::None;
 
std::string FormatValue(const int8_t& v)
{
  return std::to_string(v);
}

std::string FormatValue(const uint8_t& v)
{
  return std::to_string(v);
}

std::string FormatValue(const int16_t& v)
{
  return std::to_string(v);
}

std::string FormatValue(const uint16_t& v)
{
  return std::to_string(v);
}

std::string FormatValue(const int32_t& v)
{
  return std::to_string(v);
}

std::string FormatValue(const uint32_t& v)
{
  return std::to_string(v);
}

std::string FormatValue(const int64_t& v)
{
  return std::to_string(v);
}

std::string FormatValue(const uint64_t& v)
{
  return std::to_string(v);
}

std::string FormatValue(const std::string& v)
{
  return v;
}

std::string FormatValue(char const* const& v)
{
  return v;
}

namespace Logme
{
  std::string FormatValue(const int8_t& v)
  {
    return ::FormatValue(v);
  }

  std::string FormatValue(const uint8_t& v)
  {
    return ::FormatValue(v);
  }

  std::string FormatValue(const int16_t& v)
  {
    return ::FormatValue(v);
  }

  std::string FormatValue(const uint16_t& v)
  {
    return ::FormatValue(v);
  }

  std::string FormatValue(const int32_t& v)
  {
    return ::FormatValue(v);
  }

  std::string FormatValue(const uint32_t& v)
  {
    return ::FormatValue(v);
  }

  std::string FormatValue(const int64_t& v)
  {
    return ::FormatValue(v);
  }

  std::string FormatValue(const uint64_t& v)
  {
    return ::FormatValue(v);
  }

  std::string FormatValue(const std::string& v)
  {
    return ::FormatValue(v);
  }

  std::string FormatValue(char const* const& v)
  {
    return ::FormatValue(v);
  }
}