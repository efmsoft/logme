# Multithread example

## Multithread

This example shows how logme can be used from multiple threads.

## What it demonstrates

- Creating a dedicated channel per worker thread
- Using `LogmeThreadChannel(...)` to route channel-less logs
- Logging with `ChannelPtr` in a multi-threaded program
- A simple throughput-style loop

## Notes

- `ChannelPtr` avoids channel lookup and is usually preferable on hot paths.
- A per-thread default channel is convenient when library code does not receive a channel explicitly.
