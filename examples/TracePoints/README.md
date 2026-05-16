# Trace Points example

## TracePoints

This example demonstrates trace points: dormant logging instructions that count calls by default and start producing normal logme output only after they are enabled through runtime control.

The example first executes the code with trace points disabled, prints the collected counters, enables all trace points with `trace enable *:*:*`, executes the same workload again, and then disables them.
