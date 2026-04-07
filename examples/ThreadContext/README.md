# ThreadContext example

## ThreadContext

This example shows thread-local logging context helpers.

## What it demonstrates

- Setting a default channel for the current thread
- Setting a default override for the current thread
- Routing logs from code that does not know anything about channels
- Mixing explicit-channel and channel-less logging

## Notes

- Thread context is convenient for libraries and worker threads where passing channel parameters through every function is undesirable.
