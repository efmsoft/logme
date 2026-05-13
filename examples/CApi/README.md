# CApi example

## CApi

This example shows how to use logme from a pure C translation unit.

## What it demonstrates

- Including the common public entry point `Logme/Logme.h` from C code
- Creating a channel from C
- Attaching console and file backends from C
- Using printf-style C logging macros
- Logging to the default channel, a named channel, and a subsystem
- Using a C override object for per-call output changes and rate/repetition limits
- Flushing and shutting down logme from C

## Notes

- The C API intentionally supports a smaller feature set than the C++ API.
- C logging macros are printf-style only. The C++ stream-style syntax is not available in C.
- Channel and subsystem routing use explicit C macro variants such as `LogmeI_Ch`, `LogmeE_Sid`, and `LogmeW_ChSid`.
- `LogmeCOverride` is mutable because repetition and frequency limits keep state between calls. Keep it alive for as long as the call site needs that state.
