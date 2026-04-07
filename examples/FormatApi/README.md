# FormatApi example

## FormatApi

This example shows the supported formatting styles.

## What it demonstrates

- `std::format`-style logging when it is enabled
- Fallback to stream / printf style when `std::format` support is disabled
- Logging both to the default channel and to a custom channel

## Notes

- The `fLogme...` macros are available only when `LOGME_DISABLE_STD_FORMAT` is not defined.
