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
  union OutputFlags
  {
    uint32_t Value;

    struct
    {
      uint32_t Timestamp : 2; // Timestamp (TimeFormat enum)
      uint32_t Signature : 1; // 'E' / 'W' / 'D' / 'C' signature
      uint32_t Location : 2; // 0 - none, 1 - short, 2 - full (Detality enum)
      uint32_t Method : 1; // "MethodName(): "
      uint32_t Eol : 1; // Append \n
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
      uint32_t : 10;
      uint32_t ProcPrint : 1; // Working in the context of Procedure::xxx
      uint32_t ProcPrintIn : 1; // Input parameters
      uint32_t None : 1; // Invalid flags bit
    };

    LOGMELNK OutputFlags();

    LOGMELNK std::string ToString(const char* separator = " ", bool brackets = false) const;
    LOGMELNK std::string TimestampType() const;
    LOGMELNK std::string LocationType() const;
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