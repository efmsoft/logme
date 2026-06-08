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
- Repeated messages from the same macro call site are skipped until the repeat counter reaches `limit`.
- Other log messages written by other source lines do not reset that call site's counter.
- The example intentionally writes a normal `LogmeI(...)` message between calls to `LogmeW_Collapse(...)` to show this behavior.
- When the limit is reached, logme prints one message with the repeat counter and then starts counting again.
- The limit is count-based, not time-based.
- logme does not currently print a final summary for pending suppressed repeats when the log is closed.

## Repeated sequences and multi-line messages

Collapse works on one formatted message at one macro call site. It does not automatically detect that several different log records form a repeated block.

For a repeated sequence such as MQTT `Connecting`, `Connected`, `Subscribe`, and `Disconnected`, either collapse each log statement separately, or write one diagnostic message containing embedded `\n` characters if the whole sequence should be treated as one block.

For messages with changing ids, use `_CollapseIgnore` to remove the volatile part from the comparison key:

```cpp
LogmeI_CollapseIgnore(
  R"(\[mqtt [0-9]+\])"
  , 20
  , "[mqtt %llu] Connecting to MQTT broker"
  , mqttId
);
```
