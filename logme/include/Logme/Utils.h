#pragma once

#include <stdint.h>
#include <string>

#include <Logme/Types.h>

namespace Logme
{
  LOGMELNK std::string dword2str(uint32_t value);

  LOGMELNK uint32_t GetCurrentProcessId();
  LOGMELNK uint64_t GetCurrentThreadId();
}
