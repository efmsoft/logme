# Feature discovery map

This page maps common logging-library terms to the actual logme mechanisms and source files. It is intended as a quick audit guide for readers comparing logme with libraries that use different terminology.

| Common term | logme mechanism | Main files | Notes |
|---|---|---|---|
| Null sink / null backend | Channel with no backends, handled by early precheck and channel state checks | `logme/include/Logme/Detail/Precheck.h`, `logme/include/Logme/Channel.h` | logme does not need a dummy backend for disabled output. A channel without output destinations can be rejected before formatting and backend dispatch. |
| Sink-level filtering | Channel-level filtering by level, active state, subsystem rules, links, and overrides | `logme/include/Logme/Channel.h`, `logme/source/Channel.cpp`, `logme/include/Logme/Detail/Precheck.h` | This is intentional. A logme channel is the routing and filtering unit, so the fast path does not enumerate backends to decide whether a record should be emitted. |
| Duplicate suppression / log once | `_Once` macros, `LOGME_ONCE4THIS`, `LOGME_ONCE4CALL`, `Override::MaxRepetitions` | `logme/include/Logme/Logme.h`, `logme/include/Logme/Override.h`, `logme/source/Override.cpp`, `examples/OnceEvery` | Implemented at the call site/override layer instead of as a backend filter. This avoids generating repeated records before they reach formatting and backends. |
| Rate limiting / throttled logging | `_Every(ms)` macros, `LOGME_EVERY4THIS(ms)`, `LOGME_EVERY4CALL(ms)`, `Override::MaxFrequency` | `logme/include/Logme/Logme.h`, `logme/include/Logme/Override.h`, `logme/source/Override.cpp`, `examples/OnceEvery` | The message can be suppressed before backend work is performed. |
| Rotating file sink | `FileBackend::MaxSize`, `FileBackend::DailyRotation`, `FileBackend::MaxParts`, `DayChangeDetector` | `logme/include/Logme/Backend/FileBackend.h`, `logme/source/Backend/FileBackend.cpp`, `logme/include/Logme/DayChangeDetector.h` | File size control and daily log switching are part of `FileBackend`. |
| Log retention / total log directory limit | `DirectorySizeWatchdog`, oldest-file cleanup, in-use file protection | `logme/include/Logme/File/DirectorySizeWatchdog.h`, `logme/source/File/DirectorySizeWatchdog.cpp`, `logme/source/File/CleanOldest.cpp` | Controls total log storage, not only the currently open file. |
| Early disabled-path filtering | `LOGME_WOULD_LOG_FIRST`, `WouldLogFirst`, `WouldLog`, channel active/filter-level checks | `logme/include/Logme/Detail/Precheck.h`, `logme/include/Logme/Detail/Dispatch.h` | Used by macros that can avoid evaluating expensive arguments or preparation code when the selected channel would not log. |
| Dynamic runtime control | Control server commands, `logmectl`, `logmeweb` | `logme/source/Control`, `tools/logmectl`, `tools/logmeweb` | Channels, backends, flags, levels, logs, subsystems, and trace points can be inspected or changed at runtime. |
| Callback sink / function appender | `CallbackBackend` calls an application function for each accepted record | `logme/include/Logme/Backend/CallbackBackend.h`, `logme/source/Backend/CallbackBackend.cpp` | Useful for embedding, tests, UI bridges, telemetry bridges, or application-owned forwarding without deriving a full backend class. |
| Windows Event Log sink | `WindowsEventLogBackend` writes records through the Windows Event Log API and supports async delivery | `logme/include/Logme/Backend/WindowsEventLogBackend.h`, `logme/source/Backend/WindowsEventLogBackend.cpp`, `logme/source/WindowsEventLog` | Intended for Windows services and enterprise deployments. On non-Windows platforms the backend is a build-compatible no-op path. |
| Structured output | `OutputFlags::Format`, text/JSON/XML conversion paths, structured-output example | `logme/include/Logme/OutputFlags.h`, `logme/source/OutputFlags.cpp`, `examples/StructuredOutput`, `tools/logmefmt` | logme can emit or convert structured log records depending on configuration and build options. |
| Trace points / dormant diagnostics | Trace point macros and runtime trace control | `logme/include/Logme/TracePoint.h`, `logme/source/TracePoint.cpp`, `logme/source/Control/Command/CmdTrace.cpp`, `examples/TracePoints` | Disabled trace points keep lightweight counters and can be enabled dynamically. |

## Backend inventory

Built-in backend classes currently include:

- `ConsoleBackend`
- `DebugBackend`
- `FileBackend`
- `SharedFileBackend`
- `BufferBackend`
- `RingBufferBackend`
- `CallbackBackend`
- `WindowsEventLogBackend`

Related managers provide asynchronous console, debugger, file, and Windows Event Log processing where supported.

## Features intentionally implemented differently

Some features are intentionally not modeled as separate backends or sink filters:

- A null backend is represented by no backends on the channel plus early checks.
- Backend-level level filtering is not the normal routing model; logme filters at the channel level to preserve the fast path.
- Duplicate suppression and rate limiting are call-site features, not backend filters.
- File rotation and retention are part of `FileBackend` and the log directory watchdog rather than separate sink types.
