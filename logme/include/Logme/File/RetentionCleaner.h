#pragma once

#include <cstddef>
#include <cstdint>
#include <regex>
#include <string>

#include <Logme/Types.h>

namespace Logme
{
  struct RetentionOptions
  {
    std::size_t MaxFiles = 0;
    std::uint64_t MaxAgeMs = 0;
    std::uint64_t MaxTotalSize = 0;

    bool IsEmpty() const
    {
      return MaxFiles == 0 && MaxAgeMs == 0 && MaxTotalSize == 0;
    }
  };

  LOGMELNK void CleanFiles(
    const std::regex& pattern
    , const std::string& keep
    , const RetentionOptions& options
  );

}
