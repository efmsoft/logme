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
  /// <summary>
  /// Converts a 32-bit unsigned value to decimal string.
  /// </summary>
  /// <param name="value">Value to convert.</param>
  /// <returns>Decimal string representation.</returns>
  LOGMELNK std::string dword2str(uint32_t value);

  /// <summary>
  /// Formats binary data as a hexadecimal and printable text dump.
  /// </summary>
  /// <param name="buffer">Pointer to the beginning of the data buffer.</param>
  /// <param name="n">Total number of bytes available in the buffer.</param>
  /// <param name="offs">Number of leading spaces added before each dump line.</param>
  /// <param name="lineLimit">Maximum number of dump lines to output; 0 means no practical line limit.</param>
  /// <returns>Formatted dump string.</returns>
  LOGMELNK std::string DumpBuffer(const void* buffer, size_t n, size_t offs, size_t lineLimit = 8);

  /// <summary>
  /// Returns current process identifier.
  /// </summary>
  /// <returns>Current process id.</returns>
  LOGMELNK uint32_t GetCurrentProcessId();

  /// <summary>
  /// Returns current thread identifier.
  /// </summary>
  /// <returns>Current thread id.</returns>
  LOGMELNK uint64_t GetCurrentThreadId();

  typedef void (*PRENAMETHREAD)(uint64_t threadID, const char* threadName);

  /// <summary>
  /// Sets callback invoked by RenameThread.
  /// </summary>
  /// <param name="p">Callback pointer; nullptr disables the callback.</param>
  LOGMELNK void SetRenameThreadCallback(PRENAMETHREAD p);

  /// <summary>
  /// Calls configured thread rename callback.
  /// </summary>
  /// <param name="threadID">Thread id passed to the callback.</param>
  /// <param name="threadName">Thread name passed to the callback.</param>
  LOGMELNK void RenameThread(uint64_t threadID, const char* threadName);

  typedef std::vector<std::string> StringArray;

  /// <summary>
  /// Splits string into tokens.
  /// </summary>
  /// <param name="s">Source string.</param>
  /// <param name="words">Output array; cleared before adding tokens.</param>
  /// <param name="delimiters">Characters treated as token separators.</param>
  /// <param name="trim">Trim whitespace from the beginning and end of each token.</param>
  /// <param name="ignoreEmpty">Skip empty tokens when true.</param>
  /// <returns>Number of tokens added to words.</returns>
  LOGMELNK size_t WordSplit(
    const std::string& s
    , StringArray& words
    , const char* delimiters = " \t"
    , bool trim = true
    , bool ignoreEmpty = true
  );

  /// <summary>
  /// Sorts non-empty trimmed lines in string in place.
  /// </summary>
  /// <param name="s">String containing lines to sort.</param>
  LOGMELNK void SortLines(std::string& s);

  /// <summary>
  /// Removes leading and trailing whitespace.
  /// </summary>
  /// <param name="str">String to trim.</param>
  /// <returns>Trimmed string.</returns>
  LOGMELNK std::string TrimSpaces(const std::string& str);

  /// <summary>
  /// Converts characters to lowercase in place.
  /// </summary>
  /// <param name="str">String to modify.</param>
  /// <returns>Reference to modified string.</returns>
  LOGMELNK std::string& ToLowerAsciiInplace(std::string& str);

  /// <summary>
  /// Replaces all occurrences of one substring with another.
  /// </summary>
  /// <param name="where">Source string.</param>
  /// <param name="what">Substring to search for; must not be empty.</param>
  /// <param name="on">Replacement string.</param>
  /// <returns>String with replacements applied.</returns>
  LOGMELNK std::string ReplaceAll(
    const std::string& where
    , const std::string& what
    , const std::string& on
  );

  /// <summary>
  /// Joins strings using separator.
  /// </summary>
  /// <param name="arr">Strings to join.</param>
  /// <param name="separator">Separator inserted between strings.</param>
  /// <returns>Joined string.</returns>
  LOGMELNK std::string Join(const StringArray& arr, const std::string separator);

#ifdef USE_JSONCPP
  /// <summary>
  /// Reads byte size option from JSON value.
  /// </summary>
  /// <param name="root">JSON object containing the option.</param>
  /// <param name="option">Option name to read.</param>
  /// <param name="def">Default value returned when the option is missing or invalid.</param>
  /// <returns>Size in bytes.</returns>
  LOGMELNK uint64_t GetByteSize(const Json::Value& root, const char* option, uint64_t def);

  /// <summary>
  /// Reads time interval option from JSON value.
  /// </summary>
  /// <param name="root">JSON object containing the option.</param>
  /// <param name="option">Option name to read.</param>
  /// <param name="def">Default value returned when the option is missing or invalid.</param>
  /// <returns>Interval in milliseconds.</returns>
  LOGMELNK uint64_t GetInterval(const Json::Value& root, const char* option, uint64_t def);
#endif

  /// <summary>
  /// Returns textual name of log level.
  /// </summary>
  /// <param name="level">Log level.</param>
  /// <returns>Level name.</returns>
  LOGMELNK std::string GetLevelName(Level level);

  /// <summary>
  /// Parses textual log level name.
  /// </summary>
  /// <param name="n">Level name to parse.</param>
  /// <param name="v">Output variable receiving parsed level value.</param>
  /// <returns>True if name was parsed.</returns>
  LOGMELNK bool LevelFromName(const std::string& n, int& v);

  /// <summary>
  /// Prints integer to character buffer.
  /// </summary>
  /// <param name="buffer">Destination buffer receiving null-terminated text.</param>
  /// <param name="size">Destination buffer size in characters, including terminating null.</param>
  /// <param name="value">Integer value to print.</param>
  /// <returns>Number of printed characters, or -1 if buffer is null or too small.</returns>
  LOGMELNK int PrintIntJeaiii(
    char* buffer
    , size_t size
    , int value
  );
}
