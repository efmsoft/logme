#pragma once

#include <cstddef>
#include <cstdarg>

namespace Logme
{
  struct Context;

  enum class FastFormatKind : uint8_t
  {
    NONE,
    LITERAL,
    FORMAT_S,
    FORMAT_D,
    FORMAT_I,
    FORMAT_U,
    FORMAT_P
  };

  struct FastFormatEntry
  {
    const char* Format;
    FastFormatKind Kind1;
    FastFormatKind Kind2;
    uint16_t Part0Len;
    uint16_t Part1Pos;
    uint16_t Part1Len;
    uint16_t Part2Pos;
    uint16_t Part2Len;
    uint16_t BufferSizeHint;
    uint8_t SpecCount;
  };

  bool TryFastFormat(
    Context& context
    , char* buffer
    , size_t bufferSize
    , const char* format
    , va_list args
    , size_t* outLen = nullptr
  );
}
