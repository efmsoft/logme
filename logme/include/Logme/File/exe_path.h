#pragma once

#include <stdio.h>
#include <string>

#include <Logme/Types.h>

namespace Logme
{
  LOGMELNK std::string GetExecutablePath();
  LOGMELNK std::string AppendSlash(const std::string& folder);
  LOGMELNK std::string GetFilePathFromFd(int fd);
  LOGMELNK std::string GetFilePathFromFile(FILE* file);

  inline bool IsAbsolutePath(const std::string& path)
  {
  #ifdef _WIN32
    return (path.length() >= 2 && path[1] == ':') || (path.length() && (path[0] == '/' || path[0] == '\\'));
  #else
    return path.length() && (path[0] == '/' || path[0] == '\\');
  #endif
  } 
}
