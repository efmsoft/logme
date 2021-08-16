#pragma once

#include <string>

namespace Logme
{
  std::string GetExecutablePath();
  std::string AppendSlash(const std::string& folder);

  inline bool IsAbsolutePath(const std::string& path)
  {
  #ifdef _WIN32
    return (path.length() >= 2 && path[1] == ':') || (path.length() && (path[0] == '/' || path[0] == '\\'));
  #else
    return path.length() && (path[0] == '/' || path[0] == '\\');
  #endif
  } 
}
