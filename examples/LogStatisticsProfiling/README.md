# Log statistics profiling example

## LogStatisticsProfiling

This example shows how to identify the exact logging call sites responsible for file-output load in a multi-threaded application.

The program starts the built-in Logme control server on port `7791` and waits before launching the worker threads. Start a `logstat` session from another terminal, press Enter in the example to generate the workload, and press Enter again after 15 to 30 seconds. The program stops all workers and flushes every file backend while statistics collection is still active.

Use the second terminal to control the profiling session:

    logmectl -p 7791 logstat reset
    logmectl -p 7791 logstat start

After the example reports that the workload has stopped and all file backends have been flushed, collect the reports:

    logmectl -p 7791 logstat status
    logmectl -p 7791 logstat stop
    logmectl -p 7791 logstat outputs --backend FileBackend --sort records --limit 10
    logmectl -p 7791 logstat outputs --backend FileBackend --sort bytes --limit 10
    logmectl -p 7791 logstat backends --sort bytes --limit 10
    logmectl -p 7791 logstat files --sort written-bytes --limit 10

## What it demonstrates

- Starting the built-in Logme control server
- Generating several distinct logging workloads from multiple worker threads
- Finding a frequent short message by sorting `logstat outputs` by record count
- Finding a less frequent large message by sorting `logstat outputs` by output bytes
- Tracing one source channel that writes through two different `FileBackend` instances
- Inspecting asynchronous file-worker batches, writes, written bytes, errors, and queue drops
- Keeping log-source statistics disabled until an explicit profiling interval is started

## Expected result

The exact counters and source line numbers depend on the collection duration and build, but the reports should have this shape:

```text
c:\Source\infra\logme\out\build\x64-Debug>logmectl -p 7791 logstat outputs --backend FileBackend --sort records --limit 10
Log backend statistics: stopped
Duration: 17.952 s
Sort: records
Backend filter: FileBackend
Total: records=38439 output-bytes=3795492

1. share=98.10% records=37707 records/s=2100.41 output-bytes=1357452 KiB/s=73.84 avg=36.0 max=36 level=INFO channel=profiling.worker backend=FileBackend
   C:\Source\infra\logme\examples\LogStatisticsProfiling\LogStatisticsProfiling.cpp:79 `anonymous-namespace'::NoisyWorker
   format: Processing item: worker=%d item=%llu

2. share=1.53% records=588 records/s=32.75 output-bytes=2433144 KiB/s=132.36 avg=4138.0 max=4138 level=INFO channel=profiling.payload backend=FileBackend
   C:\Source\infra\logme\examples\LogStatisticsProfiling\LogStatisticsProfiling.cpp:90 `anonymous-namespace'::NoisyWorker
   format: Received payload: worker=%d size=%zu data=%s

3. share=0.19% records=72 records/s=4.01 output-bytes=2448 KiB/s=0.13 avg=34.0 max=34 level=INFO channel=profiling.fanout backend=FileBackend
   C:\Source\infra\logme\examples\LogStatisticsProfiling\LogStatisticsProfiling.cpp:101 `anonymous-namespace'::NoisyWorker
   format: Worker status: worker=%d item=%llu

4. share=0.19% records=72 records/s=4.01 output-bytes=2448 KiB/s=0.13 avg=34.0 max=34 level=INFO channel=profiling.fanout backend=FileBackend
   C:\Source\infra\logme\examples\LogStatisticsProfiling\LogStatisticsProfiling.cpp:101 `anonymous-namespace'::NoisyWorker
   format: Worker status: worker=%d item=%llu

c:\Source\infra\logme\out\build\x64-Debug>logmectl -p 7791 logstat outputs --backend FileBackend --sort bytes --limit 10
Log backend statistics: stopped
Duration: 17.952 s
Sort: output-bytes
Backend filter: FileBackend
Total: records=38439 output-bytes=3795492

1. share=64.11% records=588 records/s=32.75 output-bytes=2433144 KiB/s=132.36 avg=4138.0 max=4138 level=INFO channel=profiling.payload backend=FileBackend
   C:\Source\infra\logme\examples\LogStatisticsProfiling\LogStatisticsProfiling.cpp:90 `anonymous-namespace'::NoisyWorker
   format: Received payload: worker=%d size=%zu data=%s

2. share=35.76% records=37707 records/s=2100.41 output-bytes=1357452 KiB/s=73.84 avg=36.0 max=36 level=INFO channel=profiling.worker backend=FileBackend
   C:\Source\infra\logme\examples\LogStatisticsProfiling\LogStatisticsProfiling.cpp:79 `anonymous-namespace'::NoisyWorker
   format: Processing item: worker=%d item=%llu

3. share=0.06% records=72 records/s=4.01 output-bytes=2448 KiB/s=0.13 avg=34.0 max=34 level=INFO channel=profiling.fanout backend=FileBackend
   C:\Source\infra\logme\examples\LogStatisticsProfiling\LogStatisticsProfiling.cpp:101 `anonymous-namespace'::NoisyWorker
   format: Worker status: worker=%d item=%llu

4. share=0.06% records=72 records/s=4.01 output-bytes=2448 KiB/s=0.13 avg=34.0 max=34 level=INFO channel=profiling.fanout backend=FileBackend
   C:\Source\infra\logme\examples\LogStatisticsProfiling\LogStatisticsProfiling.cpp:101 `anonymous-namespace'::NoisyWorker
   format: Worker status: worker=%d item=%llu
```

The `records` report should place `Processing item: worker=%d item=%llu` first, while the `bytes` report should place `Received payload: worker=%d size=%zu data=%s` first.

## Notes

- The intentionally noisy call sites are demonstration code, not recommended production logging patterns.
- Compare both `--sort records` and `--sort bytes`. A large number of short records can be expensive even when disk bandwidth is low.
- Statistics collection is opt-in. Start it before pressing Enter to launch the workload, and stop it only after the example reports that all file backends have been flushed.
- The example creates `logstat-worker.log`, `logstat-payload.log`, `logstat-fanout-a.log`, and `logstat-fanout-b.log` in the current working directory.
