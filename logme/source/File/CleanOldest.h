#pragma once

#include <regex>
#include <string>

namespace Logme
{
  void CleanFiles(
    const std::regex& pattern
    , const std::string& keep
    , size_t limit
  );
}