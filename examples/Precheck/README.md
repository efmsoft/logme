# Precheck example

## Precheck

This example shows how Logme can skip unnecessary work before building a log record.

## What it demonstrates

- Early check for regular `LogmeI(...)` calls when the first argument is a `ChannelPtr`
- `LogmeI_Do(...)` for preparation code that should run only when the record will really be emitted
- The same idea for `std::format` style logging through `fLogmeI_Do(...)` when it is enabled
- The practical difference between an inactive channel and an active visible channel

## Why it matters

In a logging library, the expensive part is often not the final write. It is the work done before the logger decides whether the message should be emitted at all:

- evaluating function calls used as log arguments
- building temporary strings and other helper objects
- running custom preparation code just to feed the logging macro

When you pass a `ChannelPtr` as the first argument, `LogmeI(...)` can check the channel state before evaluating the remaining arguments.

When you need even more control, `LogmeI_Do(...)` lets you put the preparation code directly into the macro. That code runs only if the logger has already decided that the message will be written.

## Notes

- Plain `LogmeI("...", ...)` cannot do this early channel-based check because there is no explicit `ChannelPtr` in the first argument.
- `LogmeI_Do(...)` can work with either `ChannelPtr` or `Logme::ID`.
- The example prints counters so the behavior is visible immediately when you run it.
