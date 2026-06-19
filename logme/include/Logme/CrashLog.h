#pragma once

#include <cstdint>

namespace Logme
{

  enum CrashOutput : std::uint32_t
  {
    CRASH_OUTPUT_NONE = 0,
    CRASH_OUTPUT_FILE = 1u << 0,
    CRASH_OUTPUT_STDERR = 1u << 1,
    CRASH_OUTPUT_STDOUT = 1u << 2,
    CRASH_OUTPUT_CONSOLE = CRASH_OUTPUT_STDERR,
  };

}
