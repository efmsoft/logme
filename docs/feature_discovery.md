# Feature discovery map

This page maps common logging-library terms to the actual logme mechanisms and source files. It is intended as a quick audit guide for readers comparing logme with libraries that use different terminology.

| Common term | logme mechanism | Main files | Notes |
|---|---|---|---|
| Null sink / null backend | Channel with no backends, handled by early precheck and channel state checks | `logme/include/Logme/Detail/Precheck.h`, `logme/include/Logme/Channel.h` | logme does not need a dummy backend for disabled output. A channel without output destinations can be rejected before formatting and backend dispatch. |
| Sink-level filtering | Channel-level filtering by level, active state, subsystem rules, links, and overrides | `logme/include/Logme/Channel.h`, `logme/source/Channel.cpp`, `logme/include/Logme/Detail/Precheck.h` | This is intentional. A logme channel is the routing and filtering unit, so the fast path does not enumerate backends to decide whether a record should be emitted. |
| Duplicate suppression / log once | `_Once` macros, `LOGME_ONCE4THIS`, `LOGME_ONCE4CALL`, `Override::MaxRepetitions` | `logme/include/Logme/Logme.h`, `logme/include/Logme/Override.h`, `logme/source/Override.cpp`, `examples/OnceEvery` | Implemented at the call site/override layer instead of as a backend filter. This avoids generating repeated records before they reach formatting and backends. |
| Rate limiting / throttled logging | `_Every(ms)` macros, `LOGME_EVERY4THIS(ms)`, `LOGME_EVERY4CALL(ms)`, `Override::MaxFrequency` | `logme/include/Logme/Logme.h`, `logme/include/Logme/Override.h`, `logme/source/Override.cpp`, `examples/OnceEvery` | The message can be suppressed before backend work is performed. |
| Repeated-message aggregation | `_Collapse`, `_CollapseEvery`, `_CollapseIgnore`, `_CollapseIgnoreEvery` macros | `logme/include/Logme/Logme.h`, `logme/include/Logme/Context.h`, `logme/source/Context.cpp`, `examples/Collapse`, `tests/Collapse` | Count-based collapse summarizes after N repeated attempts. Time-based collapse summarizes after the interval elapses. Ignore variants normalize volatile substrings before comparing messages. |
| Rotating file sink | `FileBackend`, `FileTimeRotationPolicy`, `FileArchivePolicy`, `on-size-limit`, `rotation`, `archive` | `logme/include/Logme/Backend/FileBackend.h`, `logme/source/Backend/FileBackend.cpp`, `logme/source/File/FileTimeRotationPolicy.*`, `logme/source/File/FileArchivePolicy.*` | Supports size-based rotation, hourly/daily/weekly/monthly time rotation, archive index recovery, and collision-safe archive naming. |
| File archive retention | `RetentionCleaner`, `retention.max-files`, `retention.max-age`, `retention.max-total-size`, `retention.clean-on-start` | `logme/source/File/RetentionCleaner.*`, `logme/source/Backend/FileBackendConfig.cpp`, `docs/file_backend_lifecycle.md` | Applies to completed archive files and protects the active file from cleanup. `max-parts` remains a legacy alias for `retention.max-files`. |
| File lifecycle observability | `FileBackend::GetCounters()`, lifecycle counters | `logme/include/Logme/Backend/FileBackend.h`, `logme/source/Backend/FileBackend.cpp`, `docs/file_backend_lifecycle.md` | Exposes size/time completion, archive creation, compression submit, and retention-run counters when `FILE_ENABLE_COUNTERS` is enabled. |
| Log directory size watchdog | `DirectorySizeWatchdog`, in-use file protection | `logme/include/Logme/File/DirectorySizeWatchdog.h`, `logme/source/File/DirectorySizeWatchdog.cpp` | Controls total log storage at directory level independently of per-FileBackend archive retention. |
| Archive compression | `compression: "gz"`, `CompressionManager`, `USE_ZLIB` | `logme/include/Logme/File/CompressionManager.h`, `logme/source/File/CompressionManager.cpp`, `docs/file_backend_lifecycle.md` | Optional gzip compression is submitted for completed archives only; the active file is not compressed. |
| Early disabled-path filtering | `LOGME_WOULD_LOG_FIRST`, `WouldLogFirst`, `WouldLog`, channel active/filter-level checks | `logme/include/Logme/Detail/Precheck.h`, `logme/include/Logme/Detail/Dispatch.h` | Used by macros that can avoid evaluating expensive arguments or preparation code when the selected channel would not log. |
| Dynamic runtime control | Control server commands, `logmectl`, `logmeweb` | `logme/source/Control`, `tools/logmectl`, `tools/logmeweb` | Channels, backends, flags, levels, logs, subsystems, and trace points can be inspected or changed at runtime. |
| Policy-aware control API | `ControlPolicy` and `Logger::Control(command, policy)` | `logme/include/Logme/ControlPolicy.h`, `logme/source/Control/ControlPolicy.cpp`, `logme/source/Control/Control.cpp` | Useful when control commands come from less-trusted sources. Existing `Logger::Control(command)` remains full-control for compatibility. |
| Startup environment control | Explicit `ApplyEnvironmentControl()` call that reads `LOGME_CONTROL` / `LOGME_CONTROL_N` and executes commands through the control API | `logme/include/Logme/EnvironmentControl.h`, `logme/source/Control/EnvironmentControl.cpp`, `examples/EnvironmentControl`, `tests/EnvironmentControl` | Environment variables are ignored unless the application explicitly calls the method and passes options/policy. Multiple commands can be separated with `;`. |
| Recent-history capture / backtrace-style log history | `RingBufferBackend` stores the last N formatted records in memory | `logme/include/Logme/Backend/RingBufferBackend.h`, `logme/source/Backend/RingBufferBackend.cpp`, `examples/DumpBuffer` | This is log history, not a call stack. It can be used to keep recent diagnostics without permanently writing verbose logs. |
| Callback sink / function appender | `CallbackBackend` calls an application function for each accepted record | `logme/include/Logme/Backend/CallbackBackend.h`, `logme/source/Backend/CallbackBackend.cpp` | Useful for embedding, tests, UI bridges, telemetry bridges, or application-owned forwarding without deriving a full backend class. |
| Windows Event Log sink | `WindowsEventLogBackend` writes records through the Windows Event Log API and supports async delivery | `logme/include/Logme/Backend/WindowsEventLogBackend.h`, `logme/source/Backend/WindowsEventLogBackend.cpp`, `logme/source/WindowsEventLog` | Intended for Windows services and enterprise deployments. On non-Windows platforms the backend is a build-compatible no-op path. |
| Structured output | `OutputFlags::Format`, text/JSON/XML conversion paths, structured-output example | `logme/include/Logme/OutputFlags.h`, `logme/source/OutputFlags.cpp`, `examples/StructuredOutput`, `tools/logmefmt` | logme can emit or convert structured log records depending on configuration and build options. |
| Thread structured fields | `ThreadFields`, `LogmeThreadFields`, `Logger::SetThreadField()` | `logme/include/Logme/ThreadField.h`, `logme/source/ThreadField.cpp`, `logme/source/Context.cpp`, `examples/StructuredOutput`, `tests/ThreadField` | Thread-local custom fields are emitted as JSON/XML properties and are ignored by plain text output. |
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
- Backend-level level filtering is not the normal routing model; logme filters at the channel level to preserve the fast path. Advanced cases can usually be modeled with channels and links, but first-class per-backend filtering is not currently available.
- Duplicate suppression and rate limiting are call-site features, not backend filters.
- File rotation, archive naming, retention, and optional compression are part of `FileBackend` lifecycle policy rather than separate sink types.


## Known gaps and roadmap candidates

These items are useful when comparing logme with libraries that expose a very broad sink/backend inventory:

- `SyslogBackend` is not implemented yet.
- `SystemdJournalBackend` is not implemented yet.
- First-class backend-level filtering is not implemented yet.
- C API wrappers for `ControlPolicy` and environment control are not implemented yet.

