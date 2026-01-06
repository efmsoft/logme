# logme

**logme** is a lightweight, cross-platform logging framework for C/C++ with **channels**, **message routing**, and **pluggable backends** (console, debugger, file, etc.). It is designed to be simple to integrate, flexible at runtime, and comfortable for day-to-day development.

> **Key idea:** messages are written to a *channel*, and the channel decides *where* they go via one or more backends. Channels can also **link** to other channels for redirection / fan-out.

---

## Features

- **Channels**: independent log streams identified by an ID.
- **Backends**: one channel can have **one or more backends** that define where messages are emitted.
- **Channel links (redirect)**: a channel can be linked to another channel; it first writes to its own backends and then forwards to the linked channel.
- **Routing & filtering**: route messages by channel / level, optionally redirect them.
- **Color output**: optional VT100/ANSI colored console output.
- **Cross-platform**: Windows + Linux.
- **`std::format` support (optional)**: enabled automatically when available; can be forced ON/OFF.

---

## Quick start

### Default channel (simplest)

```cpp
#include <logme/logme.h>

int main()
{
  fLogmeI("Hello from default channel");
  fLogmeW("Warning: {}", 123); // if std::format API is enabled
  return 0;
}
```

> The default channel is created automatically.
> On Windows it typically includes a console backend and a debugger backend (via `OutputDebugStringA`).

---

### Create a channel and add a backend (recommended)

If you create a new channel, **you must attach at least one backend**, otherwise no output will be produced.

```cpp
#include <logme/logme.h>
#include <logme/backends/console_backend.h>

int main()
{
  auto ch = Logme::Instance->CreateChannel("net");
  ch->AddBackend(std::make_shared<ConsoleBackend>(ch));

  fLogmeI(ch, "Connected: {}", "127.0.0.1");
  return 0;
}
```

---

### Channel link (redirect)

A linked channel forwards messages to another channel after its own backends:

```cpp
#include <logme/logme.h>
#include <logme/backends/console_backend.h>

int main()
{
  auto root = Logme::Instance->CreateChannel("root");
  root->AddBackend(std::make_shared<ConsoleBackend>(root));

  auto http = Logme::Instance->CreateChannel("http");
  http->AddLink(root);

  // Goes to http backends (if any), then to root
  fLogmeI(http, "GET {} -> {}", "/index.html", 200);
  return 0;
}
```

---

## Concepts

### Channel

A channel is a named stream. You can:
- create it with `CreateChannel(id)`,
- add one or more backends with `AddBackend(...)`,
- optionally link it to another channel with `AddLink(...)`.

### Backend

A backend is an output sink (console, debugger, file, etc.).
Backends define:
- output destination,
- formatting style (colors, timestamps, prefixes).

### Link (redirect)

A link connects a channel to another channel:
- the channel writes to its own backends,
- then forwards the message to the linked channel.

---

## Installation

### Option A: CMake FetchContent

```cmake
include(FetchContent)

FetchContent_Declare(
  logme
  GIT_REPOSITORY https://github.com/efmsoft/logme.git
  GIT_TAG        v1.0.0  # use a release tag when available
)

FetchContent_MakeAvailable(logme)

target_link_libraries(your_target PRIVATE logme)
```

---

### Option B: Add as a subdirectory

```cmake
add_subdirectory(external/logme)
target_link_libraries(your_target PRIVATE logme)
```

---

## Build

### Configure and build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### Tests

```bash
cmake -S . -B build -G Ninja -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

---

## CMake options

| Option | Values | Default | Description |
|------|--------|---------|-------------|
| `LOGME_STD_FORMAT` | `AUTO`, `ON`, `OFF` | `AUTO` | Enable/disable `std::format` API. `AUTO` enables it when `<format>` is available. |
| `LOGME_DISABLE_STD_FORMAT` | `0/1` | internal | Legacy/internal switch; prefer `LOGME_STD_FORMAT`. |
| `BUILD_TESTING` | `ON/OFF` | `OFF` | Build tests. |
| `BUILD_SHARED_LIBS` | `ON/OFF` | `OFF` | Build shared library (if supported). |

---

## Requirements and compatibility

### Language standard

- C++17 or newer (see project CMake configuration).

### `std::format` support

`std::format` requires a standard library that provides the `<format>` header.

Typical cases:
- **Ubuntu 22.04 (GCC 11)**: `<format>` is not available → format disabled.
- **Debian / GCC 13+**: `<format>` is available → format enabled.

You can explicitly control this behavior:
- `-DLOGME_STD_FORMAT=OFF` — force-disable `std::format`.
- `-DLOGME_STD_FORMAT=ON` — require `std::format` (configuration will fail if unavailable).

---

## Examples

See the `Examples/` directory for small runnable samples:
- basic console logging,
- format API usage,
- channel routing and redirect.

---

## Roadmap

- GitHub Releases and changelog.
- CI matrix (Linux GCC/Clang, Windows MSVC).
- Packaging (vcpkg / Conan).

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
