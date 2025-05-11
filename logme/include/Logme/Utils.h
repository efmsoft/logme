#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#include <Logme/Types.h>

namespace Logme
{
  LOGMELNK std::string dword2str(uint32_t value);

  LOGMELNK uint32_t GetCurrentProcessId();
  LOGMELNK uint64_t GetCurrentThreadId();

  typedef void (*PRENAMETHREAD)(uint64_t threadID, const char* threadName);
  LOGMELNK void SetRenameThreadCallback(PRENAMETHREAD p);
  LOGMELNK void RenameThread(uint64_t threadID, const char* threadName);

  typedef std::vector<std::string> StringArray;

  LOGMELNK size_t WordSplit(
    const std::string& s
    , StringArray& words
    , const char* delimiters = " \t"
    , bool trim = true
    , bool ignoreEmpty = true
  );

  LOGMELNK void SortLines(std::string& s);

  LOGMELNK std::string TrimSpaces(const std::string& str);

  LOGMELNK std::string& ToLowerAsciiInplace(std::string&);
}
