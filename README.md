# logme

![Colored console output example](docs/assets/example.png)

**logme** is a lightweight, cross-platform C/C++ logging framework focused on **channels**, **multiple backends**, and **runtime control**.

- 📚 **Documentation (Wiki):** https://github.com/efmsoft/logme/wiki
- 💬 **Support / feedback:** GitHub issues and discussions are welcome.

---

## Key features

- **Channels**: logically separate log streams by subsystem / module / feature.
- **Multiple backends per channel**: console, debugger, files, etc.
- **Links between channels**: redirection / fan-out without duplicating backend setup.
- **C-style, stream-style, and (optionally) `std::format` style APIs**.
- **Cross-platform**: Windows / Linux.
- **Runtime control** via the control server and `logmectl` (enable/disable channels, manage backends, flags, levels, subsystems).

---

## Quick start

### Minimal usage (out of the box)

The simplest possible usage. This works **out of the box** and requires no configuration.

```cpp
#include <Logme/Logme.h>

int main()
{
  LogmeI("Hello from logme (%s style)", "C");
  LogmeW() << "Hello from logme (C++ stream style)";

#ifndef LOGME_DISABLE_STD_FORMAT
  fLogmeE("Hello from logme ({} style)", "std::format");
#endif

  return 0;
}
```

This produces colored console output similar to the following:

![Output result](docs/assets/base.png)

## Configuration files (recommended)

For most applications, logme is configured via a configuration file:

- channels
- backends
- links
- levels
- flags
- subsystems

This lets you adjust logging behavior at runtime or between runs without recompiling.

For details and examples, see the project Wiki.

---

### Programmatic configuration (advanced)

This section demonstrates configuring logme directly from C++.
It is useful for small tools, embedded scenarios, or when configuration files are not desired.

```cpp
#include <Logme/Logme.h>
#include <Logme/Backend/ConsoleBackend.h>

#include <memory>

int main()
{
  auto ch = Logme::Instance->CreateChannel("http");
  ch->AddBackend(std::make_shared<Logme::ConsoleBackend>(ch));

  fLogmeI(ch, "GET {} -> {}", "/index.html", 200);
  return 0;
}
```

---

## Runtime control (logmectl)

If you build the **DynamicControl** example, it starts a control server and prints from two worker threads.

There are **two channels** (`ch1`, `ch2`), and **two subsystems per channel**.
Initially the channels are inactive because they have **no backends attached**.

Activate a channel by adding a backend, for example:

```text
logmectl -p <port> backend --channel ch1 --add console
```

When the "block reported subsystems" mode is enabled, you will see messages sent to the channel **and** to subsystems.

Switch the mode off:

```text
logmectl -p <port> subsystem --unblock-reported
```

Then only messages with the **channel** specified will be visible.
To see messages with both **channel and subsystem** again, report a subsystem:

```text
logmectl -p <port> subsystem --report t1s1
```

---

## Concepts

### Channel

A **channel** is where you write messages. A channel can have:

- one or more **backends** (output destinations),
- optional **links** to other channels (redirect / fan-out).

### Backend

A **backend** defines where messages go (console, debugger, file, etc.).
Multiple backends can be attached to the same channel.

### Link (redirect)

A **link** forwards messages from one channel to another after local backends are processed.
This is useful for building routing trees (e.g., `http` → `root`).

---

## Installation

### Option A: CMake FetchContent

```cmake
include(FetchContent)

FetchContent_Declare(
  logme
  GIT_REPOSITORY https://github.com/efmsoft/logme.git
  GIT_TAG        main  # Prefer a release tag when available
)

FetchContent_MakeAvailable(logme)

target_link_libraries(your_target PRIVATE logme)
```

### Option B: Add as a subdirectory

```cmake
add_subdirectory(external/logme)

target_link_libraries(your_target PRIVATE logme)
```

---

## Build

### Configure and build

```bash
cmake -S . -B build
cmake --build build
```

### Tests

```bash
cmake -S . -B build -DLOGME_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

---

## CMake options

- `LOGME_BUILD_EXAMPLES` (ON/OFF)
- `LOGME_BUILD_TESTS` (ON/OFF)
- `LOGME_BUILD_STATIC` (ON/OFF)
- `LOGME_BUILD_DYNAMIC` (ON/OFF)
- `LOGME_STD_FORMAT` (`AUTO`, `ON`, `OFF`)

---

## Requirements and compatibility

### Language standard

- C++20 is used.

### `std::format` support

`std::format` is optional. If your standard library does not provide `<format>`, disable it via:

- `-DLOGME_STD_FORMAT=OFF` (or define `LOGME_DISABLE_STD_FORMAT`)

---

## Examples

- `Examples/Basic`
- `Examples/DynamicControl`

---

## Roadmap

- More backends and routing presets
- Improved documentation and more integration examples

---

## Contributing

Contributions are welcome via issues and pull requests.

Please include:
- OS and compiler version,
- CMake options used,
- minimal reproduction steps if applicable.

---

## License

See the `LICENSE` file.

