# On-demand log-source profiling

`logstat` is an on-demand profiler built into logme. It identifies the source
locations, channels, routes and backends responsible for logging load while the
application continues to run normally.

The feature is intended for situations where a system profiler shows work in a
logging backend, for example `FileBackend`, but cannot answer which of thousands
of log statements caused that work. Disabling logging globally may hide the CPU
or I/O cost, but it also removes the diagnostics needed to investigate the
application. `logstat` provides a narrower and measurable alternative.

Collection is disabled by default and is controlled through the regular logme
control interface, normally with `logmectl`.

## What is measured

The profiler observes four related layers:

1. **Log site** — the C or C++ source location that created the record.
2. **Source channel** — the channel that accepted the record before links and
   backend fan-out.
3. **Destination channel and backend** — every built-in backend that accepted
   the formatted record after routing.
4. **Asynchronous FileBackend runtime** — batching, successful file writes,
   write failures and queue drops after records entered the file queue.

Two metrics should normally be inspected independently:

- **records** finds very frequent messages. A large number of small records can
  spend CPU on formatting, queueing, synchronization and worker wakeups even
  when the total byte rate is modest;
- **bytes** finds large records, payload dumps and other sources of high output
  volume.

One source record can produce multiple backend records when channel links or
multiple backends fan it out. This is why source-site and backend-output reports
are separate.

## Quick start

Start a fresh interval, reproduce the workload, stop collection and inspect the
preserved result:

```bash
PORT=7791

logmectl -p "$PORT" logstat start

# Reproduce the workload for a representative interval.

logmectl -p "$PORT" logstat stop
logmectl -p "$PORT" logstat backends --sort bytes --limit 20
logmectl -p "$PORT" logstat outputs --backend FileBackend --sort bytes --limit 30
logmectl -p "$PORT" logstat outputs --backend FileBackend --sort records --limit 30
logmectl -p "$PORT" logstat files --sort written-bytes --limit 20
```

`start` starts a new session and resets previous counters. `stop` disables
collection but preserves the current session so that several reports can be
queried without the values changing.

For performance investigations, collect at least two intervals:

- an **idle interval**, when the application is running but not processing the
  workload under investigation;
- a **representative workload interval** with normal traffic;
- optionally, a **problem interval** that reproduces the reported slowdown.

Comparing these intervals separates periodic background logging from logging
caused by actual requests.

## Command reference

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

### `logstat start`

Starts a new collection session. Existing counters are discarded before the
profiler becomes active.

### `logstat stop`

Stops collection and preserves the latest counters and interval duration.
Reports remain available until the next `start`, `reset`, or process restart.

### `logstat status`

Displays whether collection is running, whether a session is available, its
duration and aggregate source, backend and asynchronous file counters.

### `logstat reset`

Clears counters without changing the active state. When collection is running,
it remains running and the interval clock restarts. When a stopped session
already exists, it remains stopped with empty counters and a zero-length
interval. Calling `reset` before the first `start` does not create a session.

### `logstat top`

Ranks individual source locations before backend routing.

```bash
logmectl -p 7791 logstat top --sort bytes --limit 30
logmectl -p 7791 logstat top --sort records --limit 30
```

Each row includes:

- source file, function and line;
- level and first observed format string;
- source channel;
- record count and records per second;
- message bytes, KiB per second, average size and maximum size;
- share of the selected sort metric.

`message-bytes` measures the rendered message body at the source site. It does
not include destination-channel prefixes or fan-out to multiple backends.

### `logstat channels`

Aggregates source-site records by the channel that first accepted them:

```bash
logmectl -p 7791 logstat channels --sort bytes --limit 30
```

Use this as a fast first pass when the application has many named diagnostic
channels.

### `logstat outputs`

Ranks source locations by the work delivered to destination backends after
channel links and fan-out:

```bash
logmectl -p 7791 logstat outputs \
  --backend FileBackend \
  --sort bytes \
  --limit 30
```

Each row identifies the source location, destination channel and backend type.
`output-bytes` includes the formatted backend record, channel prefixes and
routing fan-out.

For `FileBackend`:

- synchronous output is counted after a successful write;
- asynchronous output is counted after the record is accepted by the file
  queue;
- when file obfuscation is enabled, the obfuscated record size is counted.

Run the report twice: once sorted by `bytes` and once by `records`. The first
finds large output producers; the second finds high-frequency producers that
may dominate CPU and synchronization overhead.

### `logstat backends`

Aggregates output by destination channel and backend type:

```bash
logmectl -p 7791 logstat backends --sort bytes --limit 30
```

This answers the first diagnostic question: which channel/backend combination
is responsible for most of the logging work? The optional `--backend` filter
can isolate one backend type.

### `logstat files`

Reports what asynchronous `FileBackend` workers did after records entered their
queues:

```bash
logmectl -p 7791 logstat files --sort written-bytes --limit 30
logmectl -p 7791 logstat files --sort batches --limit 30
logmectl -p 7791 logstat files --sort errors --limit 30
logmectl -p 7791 logstat files --sort dropped-bytes --limit 30
```

The report contains:

- records and bytes accepted by the queue;
- worker batches, write operations, buffers and input bytes;
- average and maximum batch sizes;
- buffers and bytes confirmed by successful file-write calls;
- failed batches, operations, buffers and affected input bytes;
- records and bytes rejected by a full or unavailable queue.

## Reading a source-site result

A typical backend-output row looks like this:

```text
1. share=10.23% records=30107 records/s=104.84
   output-bytes=2536226 KiB/s=8.62 avg=84.2 max=165
   level=INFO channel=policy backend=FileBackend
   Entity/Condition.cpp:137 GetValue
   format: [line:%i col:%i] evaluating: %s
```

This means that one source location generated 30,107 file-backend records during
the interval and accounted for 10.23% of the selected output-byte total. The
`format` line is the format template captured from the call site, not a rendered
message containing runtime values.

Several adjacent rows with identical record counts often indicate a verbose
trace chain: every operation emits the same sequence of messages. In that case,
changing one isolated statement may not be sufficient; the whole trace sequence
or its channel/level policy should be reviewed.

## Interpreting asynchronous file statistics

### Accepted bytes closely match written bytes

The file worker is keeping up. The dominant cost is likely the number or size of
records produced by the application. Use `logstat outputs` to find the source
sites.

### Many records and small batches

The worker is waking frequently and writing small groups. The application may
be producing fragmented output, or the backend batching policy may need review.
Sort source output by `records` before changing the backend.

### Accepted bytes exceed written bytes

Data may still be queued, especially if the workload stopped immediately before
`logstat stop`. Flush application logs before stopping when a completed interval
is required. A persistent difference can also indicate write failures.

### Queue-dropped values are non-zero

The asynchronous queue could not accept all output. Some log records were lost.
This is a stronger signal than high CPU usage and should be investigated
immediately.

### Write errors are non-zero

Check filesystem availability, permissions, free space, rotation and archive
operations. Do not assume that reducing application logging alone will fix an
I/O failure.

Successful byte counts are exact for completed `WriteAll` or `WriteAllVector`
calls. If a low-level write partially succeeds and then fails, failed input bytes
describe the affected input batch and may be larger than the number of bytes
that remained unwritten.

## C and C++ call-site identification

Standard C++ logging macros use a static context cache associated with each macro
expansion. Standard native C macros also create a site-specific
`LogmeCContextCache`, so both APIs are separated by source file, function and
line.

Direct calls to legacy `LogmeWrite*` functions that do not provide a
`LogmeCContextCache` remain grouped under the shared C API context. Applications
that need exact attribution for direct C calls can use the site-aware C entry
points used by the standard macros.

For C++ stream-style output, the report may display:

```text
format: <stream or empty>
```

because there is no single printf-style format template to capture.

## Data handling and limitations

- Statistics are stored in process memory and are not persisted across process
  restart.
- The profiler stores source metadata and the first observed format template. It
  does not retain rendered message bodies or runtime argument values.
- `CallbackBackend` records calls but reports zero output bytes because logme
  cannot know what the application callback does with the context.
- Backend names identify backend types. Multiple instances of the same type are
  separated internally and aggregated as documented by each report.
- `format json logstat ...` uses the standard control JSON envelope and currently
  returns the human-readable report in `data.text`.
- Results are diagnostic counters, not a replacement for an operating-system CPU,
  disk or lock profiler. Use both views together: the system profiler identifies
  the expensive logging subsystem, while `logstat` attributes that cost to code.

## Disabled-path performance

The profiler is designed so that inactive collection has minimal effect on the
normal logging path.

While collection is disabled, it performs no:

- call-site registration;
- statistics counter updates;
- mutex acquisition;
- memory allocation;
- time query;
- map or string lookup;
- source metadata copying.

For a record that has already passed normal logme filtering and formatting, the
source-site path adds one relaxed atomic pointer load and a normally predicted
null branch. Each built-in backend that accepts the record adds the same kind of
check. The asynchronous file worker performs one active-profiler check per ready
batch, not per record. Existing early level/channel rejection happens before
these checks.

When collection is active, counters use relaxed atomic operations on established
sites. Registration and allocation occur only when a source site, output route or
file backend is first observed in the current profiler generation.

## Control policy

The control policy field `AllowLogStatistics` can disable all `logstat` commands
independently of other runtime-control operations. It is enabled by the built-in
Full, Safe and Diagnostic policies.

See also:

- [Control API](control_api.md)
- [Feature discovery map](feature_discovery.md)
- [File backend lifecycle policies](file_backend_lifecycle.md)
