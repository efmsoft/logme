# TypicalUsage examples

This directory shows the three common ways to use logme in an application. The examples are intentionally separate because these scenarios should not be mixed in one startup path.

## 1. Out-of-box usage

`OutOfBox.cpp` uses logme without explicit configuration. It writes to the default channel and is suitable for small tools, quick diagnostics, and first integration tests.

```cpp
LogmeI("application started with default logme configuration");
LogmeW("request %d took longer than expected", requestId);
```

Run:

```bash
./TypicalUsageOutOfBox
```

## 2. Code configuration

`CodeConfiguration.cpp` configures logging from C++ code. It creates an application channel, attaches console and file backends, sets the filter level, and then logs through that channel.

Use this style when the logging topology is part of the application logic or when the application does not use JSON configuration.

Run:

```bash
./TypicalUsageCodeConfiguration
```

## 3. Configuration file

`ConfigFile.cpp` loads logging setup from `typical-usage.json` or from a path passed on the command line. It does not create channels or backends in code after loading the file.

Use this style for production applications where channels, backends, levels, and runtime behavior should be adjustable without recompiling the application.

Run:

```bash
./TypicalUsageConfigFile typical-usage.json
```

Loading a JSON configuration file requires logme to be built with JsonCpp support.

## Build

```bash
cmake -S . -B build -DLOGME_BUILD_EXAMPLES=ON
cmake --build build
```

The binaries are placed in the examples output directory used by the main build.
