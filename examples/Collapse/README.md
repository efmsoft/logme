# Collapse example

## Collapse

This example shows how to collapse repeated log messages.

## What it demonstrates

- `LogmeD_Collapse(...)` / `LogmeD_CollapseIgnore(...)`
- `LogmeI_Collapse(...)` / `LogmeI_CollapseIgnore(...)`
- `LogmeW_Collapse(...)` / `LogmeW_CollapseIgnore(...)`
- `LogmeE_Collapse(...)` / `LogmeE_CollapseIgnore(...)`
- `LogmeC_Collapse(...)` / `LogmeC_CollapseIgnore(...)`

## Notes

- `LogmeX_Collapse(limit, ...)` uses the formatted message text as the repeat key.
- `LogmeX_CollapseIgnore(ignoreRegex, limit, ...)` removes all substrings matching `ignoreRegex` from the formatted message before comparing it with the previous message.
- `LogmeX_CollapseIgnore(...)` is intended for volatile fields such as request IDs, correlation IDs, timestamps, or counters.
- `LogmeX_CollapseIgnore(...)` prints the original formatted message. The regular expression is used only for comparison.
- `X` is one of the usual log levels: `D`, `I`, `W`, `E`, or `C`.
- The first message is printed immediately.
- Repeated messages are skipped until the repeat counter reaches `limit`.
- When the limit is reached, logme prints one message with the repeat counter and then starts counting again.
