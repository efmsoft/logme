#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#ifdef USE_JSONCPP
#include <json/json.h>
#endif

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

  LOGMELNK std::string ReplaceAll(
    const std::string& where
    , const std::string& what
    , const std::string& on
  );

  LOGMELNK std::string Join(const StringArray& arr, const std::string separator);

#ifdef USE_JSONCPP
  LOGMELNK uint64_t GetByteSize(const Json::Value& root, const char* option, uint64_t def);
  LOGMELNK uint64_t GetInterval(const Json::Value& root, const char* option, uint64_t def);
#endif

  LOGMELNK std::string GetLevelName(Level level);
  LOGMELNK bool LevelFromName(const std::string& n, int& v);
}
