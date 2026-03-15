#pragma once

#include <cstddef>
#include <cstdarg>

namespace Logme
{
  bool TryFastFormat(
    char* buffer
    , size_t bufferSize
    , const char* format
    , size_t formatLen
    , va_list args
  );
}
