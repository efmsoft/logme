# Crash logging

Crash logging is separate from normal logme logging.

The regular logging path (`LogmeI`, `LogmeE`, `LogmeC`, stream-style macros, format-style macros, channels, backends, runtime control, rotation, retention, and asynchronous output) is designed for normal application code. It is also the right path for controlled fatal conditions where the process is still executing normally.

A signal handler or crash handler is different. The process may have been interrupted while it held a lock, while it was inside the allocator, while it was writing a file, or while it was already inside the logger. Calling the full logging pipeline from that context can deadlock or fail before the last diagnostic is written.

For that case logme provides a low-level crash logging path.

## Prepare crash output

Open the crash file while the process is still healthy:

```cpp
Logme::Instance->OpenCrashLog("crash.log");
```

By default crash output goes to stderr. A process can change the default crash targets:

```cpp
Logme::Instance->SetCrashOutputMask(
  Logme::CRASH_OUTPUT_FILE | Logme::CRASH_OUTPUT_STDERR
);
```

Supported targets are:

| Flag | Destination |
|---|---|
| `CRASH_OUTPUT_FILE` | The file opened by `OpenCrashLog()` |
| `CRASH_OUTPUT_STDERR` | Standard error |
| `CRASH_OUTPUT_STDOUT` | Standard output |
| `CRASH_OUTPUT_CONSOLE` | Alias for `CRASH_OUTPUT_STDERR` |

`LogmeCrashToFile` does not open a file during the crash. It writes only to a file that was already prepared with `OpenCrashLog()`. This is intentional. Opening files, creating directories, constructing paths, or attaching backends during a crash would turn the crash path back into normal logging.

## Formatted crash logging

Use `LogmeCrash` when simple C-style formatting is still acceptable:

```cpp
LogmeCrash("fatal error: code=%d", code);
LogmeCrashToFile("fatal error: code=%d", code);
LogmeCrashToStderr("fatal error: code=%d", code);
LogmeCrashToStdout("fatal error: code=%d", code);
```

`LogmeCrash` formats into a fixed internal buffer using C `printf`-style formatting and then writes the resulting bytes directly to the selected crash output. It does not use channels, backends, C++ streams, `std::format`, fmt, rotation, retention, output flags, or the async queue.

This path is still less strict than the raw path because formatting is performed. Use it for emergency situations where the process is not fully destroyed and simple formatting is useful.

## Raw crash logging

Use `LogmeCrashRaw` for the strictest path:

```cpp
LogmeCrashRaw("SIGABRT received\n");
LogmeCrashRawToFile("SIGABRT received\n");
LogmeCrashRawToStderr("SIGABRT received\n");
LogmeCrashRawToStdout("SIGABRT received\n");
```

The raw macros are intended for string literals. The size is known at compile time, so the macro does not need `strlen()` and does not build a `std::string`.

Use raw crash logging for the most constrained contexts, such as a minimal POSIX signal handler.

## Advanced target selection

The shortcut macros write to one target. The mask-based forms are available when a call site wants to select or combine targets explicitly:

```cpp
LogmeCrashTo(Logme::CRASH_OUTPUT_FILE | Logme::CRASH_OUTPUT_STDERR,
             "signal received: %d", signum);

LogmeCrashRawTo(Logme::CRASH_OUTPUT_FILE | Logme::CRASH_OUTPUT_STDERR,
                "SIGABRT received\n");
```

## Example signal handler

Prepare the file during startup:

```cpp
Logme::Instance->OpenCrashLog("crash.log");
```

Then keep the handler minimal:

```cpp
static void HandleSigabrt(int)
{
  LogmeCrashRawToStderr("SIGABRT received\n");
  LogmeCrashRawToFile("SIGABRT received\n");

  ::signal(SIGABRT, SIG_DFL);
  ::abort();
}
```

If the handler needs a signal number and the process state is still good enough for simple C formatting, use the formatted crash path:

```cpp
static void HandleSignal(int signum)
{
  LogmeCrashToStderr("signal received: %d", signum);
  LogmeCrashToFile("signal received: %d", signum);

  ::signal(signum, SIG_DFL);
  ::abort();
}
```

## Relationship to fatal handling

`LogmeC`, `LogmeCheck`, `CHECK`, and `LOG(FATAL)` are controlled fatal paths. They write through the normal logging pipeline. If a fatal handler is installed, logme calls `FlushAll()` before invoking it.

Crash logging is different. `LogmeCrash` and `LogmeCrashRaw` bypass the normal logging pipeline and write directly to prepared crash outputs.

Use this rule:

```cpp
LogmeC        // controlled fatal path through normal logging
LogmeCrash    // emergency path with simple C formatting
LogmeCrashRaw // minimal literal emergency marker
```

## Limitations

Crash logging is an emergency best-effort mechanism, not a delivery guarantee. The process can still terminate before the operating system accepts the write. The disk can be full. stderr can be redirected to a broken pipe. The process can be corrupted beyond recovery.

The crash path intentionally does not support:

- channels
- backends
- runtime control
- output flags
- JSON/XML formatting
- C++ streams
- `std::format` or fmt
- rotation or retention
- async queues
- automatic file opening during the crash

Those features belong to the normal logging path. Crash logging exists so a process can still attempt to leave a small emergency marker when the normal path is too expensive or unsafe to call.
