# Overview

logme is a C/C++ logging framework built around explicit routing.

Instead of relying on a single global logger, logme uses:
- Channels as message sources
- Backends as output sinks
- Links as routing connections

This model scales well from small tools to large systems
where logging must be controlled precisely.
