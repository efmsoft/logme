#pragma once

#include <cstddef>
#include <cstdarg>

namespace Logme
{
  bool TryFastFormat(
    char* buffer
    , size_t bufferSize
    , const char* format
    , va_list args
  );
}
