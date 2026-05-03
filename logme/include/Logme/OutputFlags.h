#pragma once

#include <map>
#include <stdint.h>
#include <string>

#include <Logme/Types.h>

#if defined(_MSC_VER)
  // Anonymous struct/union inside union is a common extension (matches existing API).
  #pragma warning(push)
  #pragma warning(disable : 4201)
#endif

#if defined(__clang__)
  #pragma clang diagnostic push
  // -pedantic complains about anonymous structs (this file relies on that for API ergonomics)
  #pragma clang diagnostic ignored "-Wpedantic"
  // These are often the *actual* warnings behind pedantic for this pattern
  #pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
  #pragma clang diagnostic ignored "-Wnested-anon-types"
#elif defined(__GNUC__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wpedantic"
#endif


namespace Logme
{
  enum OutputField
  {
    OUTPUT_FIELD_TIMESTAMP,
    OUTPUT_FIELD_LEVEL,
    OUTPUT_FIELD_PROCESS_ID,
    OUTPUT_FIELD_THREAD_ID,
    OUTPUT_FIELD_CHANNEL,
    OUTPUT_FIELD_SUBSYSTEM,
    OUTPUT_FIELD_FILE,
    OUTPUT_FIELD_LINE,
    OUTPUT_FIELD_METHOD,
    OUTPUT_FIELD_MESSAGE,
    OUTPUT_FIELD_DURATION,
    OUTPUT_FIELD_COUNT
  };

  typedef std::map<OutputField, std::string> OutputFieldNameMap;

  /// <summary>Sets the structured output field name used by JSON/XML formatters.</summary>
  LOGMELNK void SetOutputFieldName(OutputField field, const char* name);
  /// <summary>Returns the structured output field name used by JSON/XML formatters.</summary>
  LOGMELNK const char* GetOutputFieldName(OutputField field);
  /// <summary>Restores default structured output field names.</summary>
  LOGMELNK void ResetOutputFieldNames();
  /// <summary>Applies multiple structured output field names at once.</summary>
  LOGMELNK void SetOutputFieldNames(const OutputFieldNameMap& names);

  /// <summary>
  /// Bit-field set controlling which parts of a log record are printed and how they are printed.
  /// It is used in channel configuration and in per-message Override objects. Individual fields are
  /// intentionally exposed for direct assignment, while Value allows copying or resetting the whole set.
  /// </summary>
  union OutputFlags
  {
    /// <summary>Raw 32-bit representation of all output flags.</summary>
    uint32_t Value;

    struct
    {
      uint32_t Timestamp : 2; // Timestamp (TimeFormat enum)
      uint32_t Signature : 1; // 'E' / 'W' / 'D' / 'C' signature
      uint32_t Location : 2; // 0 - none, 1 - short, 2 - full (Detality enum)
      uint32_t Method : 1; // "MethodName(): "
      uint32_t Eol : 1; // Append 

      uint32_t ErrorPrefix : 1; // Append "Error: " / "Critical: "
      uint32_t Duration : 1; // Duration of procedures
      uint32_t ThreadID : 1; // Thread ID
      uint32_t ProcessID : 1; // Process ID
      uint32_t Channel : 1; // Print name of channel
      uint32_t Highlight : 1; // CONSOLE BACKEND ONLY: Color highlighting in console
      uint32_t Console : 3; // CONSOLE BACKEND ONLY:Console output type (ConsoleStream enum)
      uint32_t DisableLink : 1; // Do not send to linked channel
      uint32_t ThreadTransition : 1; // Log thread name transitions
      uint32_t Subsystem : 1; // Print name of subsystem
      uint32_t Format: 2; // Format of output: text / json / xml (OutputFormat enum)
      uint32_t : 8;
      uint32_t ProcPrint : 1; // Working in the context of Procedure::xxx
      uint32_t ProcPrintIn : 1; // Input parameters
      uint32_t None : 1; // Invalid flags bit
    };

    /// <summary>
    /// Creates the default output flag set used by channels: timestamp, level signature, method name,
    /// error prefix, console highlighting, line ending, and thread-transition logging are enabled.
    /// </summary>
    LOGMELNK OutputFlags();

    /// <summary>
    /// Converts enabled flags to a readable string, optionally wrapping the result in brackets.
    /// </summary>
    LOGMELNK std::string ToString(const char* separator = " ", bool brackets = false) const;
    /// <summary>Returns the readable name of the selected timestamp mode.</summary>
    LOGMELNK std::string TimestampType() const;
    /// <summary>Returns the readable name of the selected source-location mode.</summary>
    LOGMELNK std::string LocationType() const;
    /// <summary>Returns the readable name of the selected console-output routing mode.</summary>
    LOGMELNK std::string ConsoleType() const;
  };

  typedef std::map<std::string, OutputFlags> OutputFlagsMap;
}

#if defined(__clang__)
  #pragma clang diagnostic pop
#elif defined(__GNUC__)
  #pragma GCC diagnostic pop
#endif

#if defined(_MSC_VER)
  #pragma warning(pop)
#endif
