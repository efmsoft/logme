# Architecture Overview

This document describes the core ideas behind **logme** and explains how channels,
backends, and links work together.

## Design goals

- Explicit routing instead of global implicit configuration
- Ability to redirect and fan-out messages at runtime
- Minimal overhead and minimal dependencies
- Predictable behavior: nothing is printed unless explicitly configured

## Channels

A **channel** is an independent log stream identified by an ID.

Important properties:
- Channels do not output anything by default
- Output appears only after at least one backend is attached
- Channels can be created at any time

This design avoids accidental logging and makes routing explicit.

## Backends

A **backend** defines *where* and *how* log messages are emitted.

Typical responsibilities:
- Output destination (console, debugger, file, etc.)
- Formatting (timestamps, colors, prefixes)
- Severity filtering

A channel may have multiple backends.

## Links (redirects)

Channels may be linked to other channels.

When a message is written:
1. The channel sends the message to its own backends
2. The message is forwarded to the linked channel (if any)

This allows:
- Centralized aggregation channels
- Hierarchical logging trees
- Selective redirection without global state

## Default channel

The default channel is created automatically.

On Windows, it typically has:
- Console backend
- Debugger backend (OutputDebugStringA)

User-created channels start with **no backends** by design.
