# logme release notes

## [1.2.0] - 2026-01-13

### Added
- Proper CMake install and export rules for `logme` and `logmed` targets.
- CMake package configuration support (`logmeConfig.cmake`, version file, and exported targets).
- Stable consumer target alias `logme::logme` for both static and shared builds.
- Improved compatibility with package managers (vcpkg overlays and ports).

### Changed
- CMake build system updated to support standard `cmake --install` workflow.
- Static and shared library builds are now fully compatible with `find_package(logme CONFIG)`.
- Installation layout aligned with GNUInstallDirs conventions.

### Fixed
- Missing `install` target in CMake/Ninja generators.
- Issues preventing successful packaging and installation via vcpkg.

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

