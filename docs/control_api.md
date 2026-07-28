# Control API

This document describes the **logme control interface** used by `logmectl` and the embedded control server.


## Control policies

The programmatic control API supports policy-aware execution:

```cpp
logger.Control(command, Logme::ControlPolicy::Safe());
logger.ApplyEnvironmentControl(options);
logger.StartControlServer(config, Logme::ControlPolicy::Safe());
logger.SetControlServerPolicy(Logme::ControlPolicy::Diagnostic());
```

The legacy overloads keep the old behavior and use full control:

```cpp
logger.Control(command);
logger.StartControlServer(config);
```

This keeps existing applications compatible, while allowing applications that expose
the control server to restrict dangerous commands such as backend changes, log-file
access, or extension commands.

The API supports two output formats:

- `text` (default): human-readable text, printed as-is.
- `json`: machine-readable JSON envelope.

## CLI

`logmectl` supports:

- `--format text|json` (default: `text`)

Example:

```text
logmectl -p 7791 subsystem
logmectl -p 7791 --format json subsystem
```

## Text format

In `--format text` mode, the client prints the server response **as-is**.

Errors are indicated by responses that start with:

```text
error: <message>
```

The server guarantees that a response is always returned (never empty).
For “empty” results the text explicitly states `none` rather than printing an empty section.

## JSON format

In `--format json` mode, the client sends the command to the server wrapped as:

```text
format json <command...>
```

The server replies with a single JSON object:

```json
{
  "ok": true,
  "error": null,
  "data": {}
}
```

### Envelope

- `ok` (boolean)  
  `true` on success, `false` on error.

- `error` (string|null)  
  `null` on success, otherwise a human-readable error message.

- `data` (object)  
  Command-specific result data. Always present.

Notes:

- In JSON mode, errors are represented only by `ok=false` and `error`.
  The server does **not** use a `error:` prefix inside JSON.
- `data` may include an optional `text` field (string) which contains the same human-readable output as the text format.
  This is intended for debugging and backward compatibility.

## Command schemas

Only fields documented below are guaranteed. New fields may be added in the future.

### `subsystem`

The command manages subsystem allow/block filters and optional per-subsystem
level overrides.

Text example:

```text
Blocked subsystems:
  CLOUD
Allowed subsystems:
  DSL
Subsystem levels:
  CLOUD: WARN
  DSL: DEBUG
```

If a section is empty, it is printed as `none`.

JSON:

```json
{
  "ok": true,
  "error": null,
  "data": {
    "blockedSubsystems": ["CLOUD"],
    "allowedSubsystems": ["DSL"],
    "subsystemLevels": {
      "CLOUD": "WARN",
      "DSL": "DEBUG"
    }
  }
}
```

Fields:

- `blockedSubsystems` (array of strings)
- `allowedSubsystems` (array of strings)
- `subsystemLevels` (object mapping subsystem names to level names)

When no entries are configured, arrays are `[]` and `subsystemLevels` is `{}`.

The blocked list has priority. When the allowed list is not empty, only listed
subsystems are logged, unless they are also blocked. Messages without a
subsystem are not affected by subsystem allow/block filtering.

A level override applies only to a named subsystem. It replaces the level of
each channel receiving the record; it does not combine with the channel level.
This allows both less restrictive and more restrictive policies:

```text
channel pe = INFO, subsystem DSL = DEBUG
channel pe = INFO, subsystem CLOUD = WARN
```

If a named subsystem has no override, the level of the current channel is used.
Channel active state and subsystem blocked/allowed filters still take priority.

Supported text commands:

```text
subsystem
subsystem --block name
subsystem --unblock name
subsystem --allow name
subsystem --disallow name
subsystem --set-level name level
subsystem --remove-level name
subsystem --clear-levels
subsystem --clear-blocked
subsystem --clear-allowed
subsystem --clear
subsystem --check name
```

`--set-level` accepts the same level names as channel level configuration.
`--clear` clears blocked, allowed, and level-override entries.

Example:

```text
subsystem --set-level DSL DEBUG
subsystem --set-level CLOUD WARN
subsystem --remove-level DSL
subsystem --clear-levels
```

`subsystem --check name` reports the effective subsystem configuration:

```text
Blocked: false
Allowed: true
Level: DEBUG
```

When no level override exists, the final line is:

```text
Level: channel default
```

In JSON mode the check result uses `blocked` and `allowed` booleans and a
`level` string. The `level` value is either the configured level name or
`"channel default"`.

### `list`

Text example:

```text
<default>
net
ssl
```

JSON:

```json
{
  "ok": true,
  "error": null,
  "data": {
    "channels": ["", "net", "ssl"]
  }
}
```

Fields:

- `channels` (array of strings)  
  The default channel is represented as an empty string `""`.

### `flags`

Text example:

```text
Flags: 0x00000001 timestamps
```

JSON:

```json
{
  "ok": true,
  "error": null,
  "data": {
    "value": 1,
    "names": ["timestamps"]
  }
}
```

Fields:

- `value` (integer)  
  Raw flags bitmask.
- `names` (array of strings)  
  Flag names (space-separated tokens from the textual representation).

### `level`

Text example:

```text
Level: info
```

JSON:

```json
{
  "ok": true,
  "error": null,
  "data": {
    "level": "info"
  }
}
```

Fields:

- `level` (string)

### `backend`, `channel`, `help`

These commands are supported in JSON mode and return the standard envelope.
For operations that only report success, `data` may be `{}`.

In the future these commands may return structured `data` objects as needed.


### `logs`

The `logs` command exposes a read-only view of log files below the current
logger home directory.

Supported text commands:

```text
logs --info
logs --tree [relative-path]
logs --tail relative-file-path [bytes]
logs --read relative-file-path [offset] [bytes]
logs --download relative-file-path
```

The command never accepts absolute paths and rejects paths that resolve outside
the logger home directory. Only files with extensions configured in
`home-directory.watch-dog.file-extension` are exposed. If the configured list is
empty, the control command uses the standard log extensions:

```text
.log .nlb .nlr .b64 .dat .csv
```

`logs --tree` returns tab-separated lines:

```text
Home directory: /var/log/my-app/
Path: logs
DIR     logs/archive
FILE    logs/app.log  123456  132456789
```

`logs --tail` returns the last part of the selected file. The optional `bytes`
argument is capped by the server to avoid returning very large files in one
response.

`logs --read` returns a bounded range of the selected file. The response starts
with a metadata header followed by the file chunk:

```text
LOGMEWEB-RANGE    offset    requested-bytes    file-size
```

`logs --download` returns the selected file encoded as base64 with a metadata
header:

```text
LOGMEWEB-DOWNLOAD-B64    file-size
```

The download response is intended for `logmeweb` and is also capped by the
server to avoid transferring unexpectedly huge files through the control
interface.

## Log-site statistics (`logstat`)

`logstat` is an on-demand profiler for identifying source locations that generate
most log records and message text. It is intended for production diagnostics when
logging appears in a CPU or I/O profile and disabling all logging would remove the
information needed to diagnose the application.

For the complete diagnostic workflow, metric interpretation, asynchronous file
analysis and disabled-path performance contract, see
[On-demand log-source profiling](log_statistics.md).

Collection is disabled by default. Start a fresh interval, reproduce the workload,
and then inspect the results:

```text
logmectl -p 7791 logstat start
logmectl -p 7791 logstat top --sort bytes --limit 30
logmectl -p 7791 logstat top --sort records --limit 30
logmectl -p 7791 logstat channels --sort bytes
logmectl -p 7791 logstat outputs --backend FileBackend --sort bytes --limit 30
logmectl -p 7791 logstat backends --sort bytes
logmectl -p 7791 logstat files --sort written-bytes --limit 30
logmectl -p 7791 logstat stop
```

Supported commands:

```text
logstat start
logstat stop
logstat status
logstat reset
logstat top [--sort bytes|records] [--limit count]
logstat channels [--sort bytes|records] [--limit count]
logstat outputs [--sort bytes|records] [--limit count] [--backend type]
logstat backends [--sort bytes|records] [--limit count] [--backend type]
logstat files [--sort written-bytes|batches|errors|dropped-bytes] [--limit count]
```

`start` resets previous counters before collection becomes active. `stop` disables
collection and preserves the latest result. `reset` clears counters and restarts the
measurement interval without changing whether collection is active.

The top-site report contains:

- source file, function and line;
- log level and first observed format string;
- accepted source channel before links and backend fan-out;
- record count and records per second;
- message bytes, KiB per second, average size and maximum size;
- share of the selected sort metric.

The channel report aggregates the same record and message-byte counters by accepted
source channel.

The output report attributes each source location to the destination channel and
built-in backend that accepted the formatted record. `output-bytes` includes channel
prefixes, output formatting and link/backend fan-out. For `FileBackend`, synchronous
writes are counted after a successful write and asynchronous writes after the data is
accepted by the file queue. With file obfuscation enabled, the encrypted record size
is counted. The backend report aggregates these counters by destination channel and
backend type. Use `--backend FileBackend` to isolate file output:

```text
logstat outputs --backend FileBackend --sort bytes --limit 30
```

The file runtime report shows what happened after records entered an asynchronous
`FileBackend` queue. It reports accepted records and bytes, worker batches and write
operations, batch sizes, bytes confirmed by successful file-write calls, failed write
operations and input bytes, and records rejected because the queue could not accept
more data. Typical use:

```text
logstat files --sort written-bytes --limit 30
logstat files --sort batches --limit 30
logstat files --sort errors --limit 30
logstat files --sort dropped-bytes --limit 30
```

The file counters use the same `start`/`stop` interval as source-site counters. Data
that remains queued when `stop` is issued appears as accepted but not yet written. To
measure a completed interval, flush the application logs before stopping collection.
Successful byte counts are exact for completed `WriteAll`/`WriteAllVector` calls. On a
low-level partial-write failure, failed input bytes describe the affected input batch
and may be larger than the number of bytes that were not physically written.

`CallbackBackend` records calls but reports zero output bytes because Logme cannot
know what the application callback does with the context. Native C logging macros
have their own static statistics cache at each expansion site and are separated by
source file, function and line in the same way as C++ macros. Direct calls to the
`LogmeWrite*` functions without a `LogmeCContextCache` remain grouped by the common
C API context; the standard C macros use the site-aware functions automatically.

`format json logstat ...` returns the standard JSON envelope and includes the report
in `data.text`.

### Disabled-path cost

The profiler performs no site registration, counter updates, locking, allocation or
time queries while collection is disabled. For a record that has already passed the
normal Logme filters and formatting path, the source-site path adds one relaxed atomic
pointer load and a normally predicted null branch. Each built-in backend that accepts
the record adds one relaxed atomic pointer load and a normally predicted null branch.
The asynchronous file worker adds one relaxed active-profiler check per ready batch;
queue-drop accounting is reached only on the exceptional append-failure path. Calls
rejected by the existing early level/channel precheck do not reach the source-site
check or any backend statistics check.

The control policy field `AllowLogStatistics` can disable `logstat` independently.
It is enabled by the built-in Full, Safe and Diagnostic policies.
