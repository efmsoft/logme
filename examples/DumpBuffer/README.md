# DumpBuffer example

## DumpBuffer

This example shows how to dump a binary buffer as text.

## What it demonstrates

- Building a small buffer with printable and non-printable bytes
- Converting the dump to a string with `Logme::DumpBuffer(...)`
- C-style logging with `%s` and `.c_str()`
- `std::format`-style logging when it is enabled
- Using different offsets and line limits

## Notes

- The example logs to the default channel and does not configure any custom channels.
- Each dump is preceded by a short description of the format and the selected offset/limit.
