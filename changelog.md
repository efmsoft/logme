## 2.4.18

### Added
- Added `FileBackend` archive lifecycle policies: size-limit rotation with `on-size-limit: "rotate"`, explicit `archive` patterns, `{index}` archive parts, `{date}` / `{datetime}` archive names, and collision-safe archive-name selection.
- Added time rotation policies for `FileBackend`: `hourly`, `daily`, `weekly`, `monthly`, plus `none` / `off` / `disabled`. The existing `daily` configuration path remains compatible.
- Added archive retention policies for completed file archives: `retention.max-files`, `retention.max-age`, `retention.max-total-size`, and `retention.clean-on-start`. The legacy `max-parts` option remains supported as an alias for `retention.max-files`.
- Added optional gzip compression for completed file archives through `compression: "gz"` / `"gzip"` and the `USE_ZLIB` CMake option. If zlib support is not compiled in, gzip compression configuration is accepted as a no-op.
- Added startup archive-index recovery, including `.gz` archive-name awareness, so restarted processes do not overwrite existing archive files.
- Added `FileArchivePolicy`, `FileTimeRotationPolicy`, `RetentionCleaner`, and `CompressionManager` building blocks behind the file backend lifecycle implementation.
- Added lifecycle counters for `FileBackend` when `FILE_ENABLE_COUNTERS` is enabled: size-limit completions, time-limit completions, archived files, compression submissions, and retention runs.
- Added dedicated tests for file archive policy, file backend configuration parsing, file backend integration scenarios, lifecycle edge cases, failure handling, and hot-path rotation behavior.

### Improved
- Unified the file-completion path used by size rotation and time rotation, so archive naming, retention, compression, and counters are handled consistently.
- Kept the normal file write hot path focused on append and cheap checks. Archive scanning, regex matching, retention cleanup, and compression submission remain on startup or file-completion paths.
- Improved `FileBackend` rotation failure behavior: if archive directory creation or active-file rename fails, the backend reopens the active log in append mode and keeps logging when possible instead of risking active-log truncation.
- Improved async file flush scheduling by tracking first-write timestamps in `BufferQueue` and using ready/current-buffer timestamp snapshots to preserve flush deadlines without the previous expensive oldest-data path.
- Improved file lifecycle documentation, examples, and wiki coverage for production rotation, retention, compression, counters, and failure behavior.

### Fixed
- Fixed Windows/MSVC warning-as-error issues found in the release matrix, including local variable shadowing in `FileBackend`.
- Fixed cross-platform test issues around Windows path separators and wide/narrow path characters.
- Fixed retention edge-case coverage around disabled retention limits, unrelated files, active-file protection, archive gaps, `.gz` collisions, and runtime archive collisions.
- Fixed gzip-compression test coverage for both zlib-enabled and zlib-disabled builds.

## 2.4.17

### Added
- Added `logmeweb`, a lightweight FastAPI-based web UI for the logme control server. It can discover running logme-enabled processes, connect to the selected target, show overview data, manage channels/backends, browse logs, execute manual commands, and control trace points from a browser.
- Added `LogmeWebManual`, a manual smoke-test host for `logmeweb`, including HTTP/HTTPS modes, password-protected control access, obfuscation mode, backend management checks, trace point checks, and log browsing checks.
- Added local control-server discovery so tools can locate running logme control servers without hard-coded host/port values. Discovery exposes only connection metadata and never publishes passwords.
- Added HTTPS support for the control/web workflow, including SSL helper APIs, generated local self-signed certificates for manual testing, and `logmectl` / DynamicControl SSL coverage.
- Added optional web UI authentication for `logmeweb`, including login sessions, secure cookie handling, IP/User-Agent session binding, failed-login throttling, configurable session timeout, and trusted reverse-proxy header support.
- Added a `logs` control command for bounded, read-only browsing/downloading/tailing of log files under the configured home directory, with extension filtering based on logme home-directory settings.
- Added `overview` control command output with aggregated channel/backend statistics, logged bytes, backend memory, obfuscation state, error-channel information, and backend type counts.
- Added `backend` control command support for runtime backend operations, including adding/removing backends and configuring backend-specific options such as async mode, file name, size limits, rotation parts, shared-file timeout, and buffer policy.
- Added Trace Points support for lightweight opt-in diagnostics that can be discovered and controlled at runtime, including counters, enable/disable by pattern, counter reset, `trace` control command support, tests, and example project.
- Added `WindowsEventLogBackend` for writing log records to Windows Event Log, including JSON configuration support, async Windows Event Log manager/factory, example project, and tests.
- Added `CallbackBackend` example and tests to demonstrate routing log records into user-provided callbacks.

### Improved
- Improved runtime control protocol behavior and command coverage, including normalized `ok` / `error:` responses that are easier for tools and the web UI to consume.
- Improved `logmeweb` UI coverage for overview, channels, backend details, backend add/delete operations, logs, trace points, discovery, password-protected targets, and manual command execution.
- Improved `DynamicControl` and `logmectl` examples/tests around SSL, password authentication, command handling, and runtime control scenarios.
- Improved backend inspection output so web/control tools can display richer ConsoleBackend, FileBackend, BufferBackend and related backend state.
- Improved build system coverage for examples, tests, tools, static/dynamic builds, Windows solution/MSBuild, Linux, macOS, CMake options, and C++20 requirements.
- Improved README and documentation around supported platforms, CMake integration, runtime control, discovery, web UI usage, HTTPS, authentication, trace points, and available examples.

### Fixed
- Fixed deadlock-prone logging paths by avoiding channel `DataLock` being held across linked-channel dispatch and filter callbacks, so recursive/channel-link logging paths do not block on the same channel lock.
- Fixed BufferQueue / async file-output deadlock and flush issues by separating ready/free/current-buffer locking, publishing the current buffer before waiting, and preventing soft-flush paths from waiting forever on data that remains in the current buffer.
- Fixed shutdown/flush handling for async backends and managers so ConsoleBackend, DebugBackend, FileBackend, and WindowsEventLogBackend can drain or unregister consistently during shutdown.
- Fixed POSIX `SIGPIPE` handling for control/network code paths.
- Fixed C++ formatting through log macros in release builds.
- Fixed password handling in web/control workflows so protected targets continue to receive the password on subsequent requests after connection.
- Fixed multiple build issues affecting examples, tests, Linux, macOS, MSVC, legacy project generation, and generated Visual Studio project files.

## 2.4.16

- Added optional fmt-based formatting backend via `LOGME_FMT_FORMAT=AUTO|ON|OFF`.
- Added release and CI coverage for full dependency builds with fmt and JsonCpp.
- Optimized printf-style fast formatting by using cached buffer size hints and avoiding unnecessary large temporary buffers for simple formats.
- Reduced extra string scanning/copying in the log context application path.
- Fixed `FileBackend::Flush()` to publish the current buffer before waiting for queued data to drain.
- Added MSVC legacy preprocessor compatibility test built with `/Zc:preprocessor-`.
- Improved IntelliSense/XML documentation for public logging macros, Logger, Channel, Backend, Procedure, Override, IDs and utility APIs.

## 2.4.15

### Added
- Added `ContextCache` support in logging macros and dispatch paths to cache per-call-site context data.
- Added `GetDefaultChannelPtr()` helper for accessing the default channel as a `ChannelPtr`.
- Added `LogmeX_Once` / `LogmeX_Every` helpers and completed once / rate-limited override support.
- Added `OnceEvery` and `DumpBuffer` examples.
- Added optional diagnostic counters for `FileBackend`, `FileManager`, and `BufferQueue` to simplify performance analysis.

### Improved
- Reduced hot-path overhead by switching selected `ChannelPtr` passing paths to references where appropriate.
- Optimized `BufferQueue` and related file-output paths.
- Reduced spurious flush requests and unnecessary `FileManager` wakeups during file logging.
- Improved support for precheck-based early logging decisions.

### Fixed
- Fixed double evaluation in logging macros.
- Fixed stream `Context` lifetime issues.
- Fixed build issues affecting general builds, including Clang and CLion configurations.

## 2.4.14

### Fixed
- Suppressed warnings when passing non-trivial objects to `fLogme*` macros.

## 2.4.13

### Added
- Added **FastFormat** optimized formatting path for common logging scenarios.
- Added protection against **recursive logging** in the same channel.

### Improved
- Improved **file output performance**, reducing overhead in heavy logging workloads.
- Optimized **FileBackend** and buffering paths to reduce contention.
- Improved **BufferQueue** behavior and allocation patterns under concurrency.
- Optimized hot-path checks for channel activity using cached / thread-local mechanisms.
- Reduced overhead of repeated `isatty` detection for console output.
- Improved console handling when output is redirected (non-TTY targets).
- Added automatic **ANSI escape stripping** for non-TTY outputs.

### Fixed
- Fixed several edge cases discovered during performance optimization work.
- Fixed minor correctness issues in logging paths and backend handling.
- Fixed small build and portability issues.

### Notes
- This release mainly focuses on **performance and hot-path optimizations**.
- No major API changes.

## 2.4.11

### Improvements
- Optimized channel behavior when the backend list is empty.
- Optimized FileBackend under intensive write load by reducing duplicate RequestFlush calls.

## 2.4.6

### Fixed
- Updated GitHub release workflow (`release.yml`): corrected Windows artifacts packaging (headers and libraries are now included in release archives).

### Notes
- No library code changes since 2.4.4.
- This release only affects CI/release packaging.

## 2.4.4

### Fixed
- Fixed CMake build directory escaping in `add_subdirectory()` calls (`../out/...` → `out/...`).
  This prevents debug/release build races when building in parallel (e.g. vcpkg).
  
## 2.4.2

### Fixed
- Fixed vcpkg CI failures when building examples/tests/tools with shared or static configurations.
- Centralized library selection via `LOGME_LINK_TARGET` for all examples, tests, and tools.
- Centralized Windows runtime DLL copy (`logmed`) using `LogmeCopyRuntime()` helper.
- Made `_LOGME_STATIC_BUILD_` consistently follow `LOGME_LINK_TARGET` (including `logmectl`).

# logme release notes

## v1.6.0  2026-01-13

### Highlights
- Fixed Windows shared builds with `tests` enabled under vcpkg: test discovery (`gtest_discover_tests`) no longer fails due to missing runtime DLLs.
- Improved build-tree runtime behavior for shared builds: required DLLs are copied next to produced executables so tests and examples can run reliably without PATH tweaks.

### Packaging / Build system
- For Windows shared builds (`USE_LOGME_SHARED=ON`), executables built in subfolders (tests/examples/tools) receive required runtime DLLs in their output directory.
- This addresses `0xc0000135` launch failures during build-time test discovery on Windows.

### vcpkg integration
- Shared builds with `examples`, `tests`, and `tools` features enabled build and execute correctly in vcpkg environments.

### Notes
- No public API changes.
- Static build behavior remains unchanged.

---

## v1.4.0  2026-01-13

### Packaging / Build system
- Fixed shared build usability on Windows: test, example, and tool executables now correctly locate `logmed.dll` at runtime.
- In shared builds, `logmed.dll` is copied next to each executable (tests, examples, tools) to ensure reliable execution during build and test discovery.
- This resolves failures of `gtest_discover_tests()` on Windows when building with shared libraries.

### vcpkg integration
- Improved compatibility with vcpkg dynamic builds when `examples`, `tests`, or `tools` features are enabled.
- Shared builds with enabled features now build and execute correctly without requiring PATH hacks or manual DLL copying.
- The build flag `USE_LOGME_SHARED` is now fully respected by all in-tree examples, tests, and tools.

### CMake / Project structure
- All examples, tests, and tools consistently select `logme` or `logmed` by name based on `USE_LOGME_SHARED`.
- No dependency on alias targets inside subprojects; direct `add_subdirectory()` usage remains fully supported.
- Static build behavior remains unchanged.

### Notes
- No public API changes.
- No behavioral changes for static builds.
- This release focuses on build robustness and package-manager friendliness, especially on Windows.

## v1.3.0

### Highlights
- Improved CMake packaging for vcpkg and other package managers: install/export rules and stable `find_package(logme CONFIG)` integration.
- Added `USE_LOGME_SHARED` switch so in-tree examples/tests/tools can link against the shared library (`logmed`) when desired (e.g., vcpkg dynamic builds), while keeping static as the default when the switch is not defined.
- Fixed GoogleTest discovery and enabled building gtest-based tests when `GTest` is available.

### Notes
- For static builds on Windows, consumers require `_LOGME_STATIC_BUILD_` (propagated via targets in installed packages).
- For shared builds, `_LOGME_DLL_BUILD_` is used only while building the DLL (dllexport) and is not propagated to consumers (dllimport).

## v1.2.0

### Highlights
- CMake and documentation improvements for easier integration.
- Build system and example updates.

## v1.1.0 (Jan 12, 2026)

### Highlights
- **Control protocol refresh:** protocol changes aimed at making runtime control more consistent and feature-complete.
- **Tooling & examples:** continued work around the control ecosystem (including `dynamic/` tooling and examples).
- **CI/build improvements:** Windows CI was updated, including adding a dedicated **MSBuild build of the repo `.sln`** and switching the other Windows job to a Ninja+MSVC flow. ([github.com](https://github.com/efmsoft/logme/commit/5b9ef6a090596e262c1ce71f903899d91713772a))

### Build / CI
- Added **Windows / MSBuild (repo `logme.sln`)** job in CI (Debug/Release). ([github.com](https://github.com/efmsoft/logme/commit/5b9ef6a090596e262c1ce71f903899d91713772a))
- Updated Windows CI build strategy to use Ninja with MSVC (`cl`) and simplified `ctest` invocation. ([github.com](https://github.com/efmsoft/logme/commit/5b9ef6a090596e262c1ce71f903899d91713772a))

### Docs / repo
- Repository documentation emphasizes that the library targets **C++20** and that Visual Studio **`.sln`** is available on Windows. ([github.com](https://github.com/efmsoft/logme.git))

> Note: if you want this section to be 100% exhaustive (feature-by-feature), I can expand it to a full changelog by walking the commit list between **v1.0.0** and **v1.1.0** and extracting the user-visible changes.

---

## v1.0.0 (Jan 6, 2026)

Initial stable public release. ([github.com](https://github.com/efmsoft/logme/releases/tag/v1.0.0))

logme is a compact cross-platform logging framework for C and C++
based on a channel-oriented architecture and flexible message routing. ([github.com](https://github.com/efmsoft/logme/releases/tag/v1.0.0))

Key features:
- Channel-based logging architecture
- Multiple logging APIs:
  - C-style macros
  - C++ stream-style interface
  - std::format-based API (when enabled)
- Multiple backends:
  - Console
  - File
  - Debugger
  - In-memory buffer
- Cross-platform support (Windows, Linux)

Public API is considered stable starting from this release. ([github.com](https://github.com/efmsoft/logme/releases/tag/v1.0.0))

