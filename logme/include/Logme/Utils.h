#pragma once

#include <stdint.h>
#include <string>

namespace Logme
{
  std::string dword2str(uint32_t value);

  uint32_t GetCurrentProcessId();
  uint64_t GetCurrentThreadId();
}
