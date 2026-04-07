# Basic example

## Basic

This example shows the most basic logme usage.

## What it demonstrates

- Stream-style logging with `LogmeI() << ...`
- Printf-style logging with `LogmeI("...", ...)`
- Logging to a custom channel
- Applying a local override to a single call
- Using `OBF(...)` for an obfuscated string literal

## Notes

- The default channel `::CH` is available automatically.
- Custom channels must be made visible by linking them to `::CH` or by attaching backends directly.
