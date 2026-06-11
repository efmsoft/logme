# Collapse example

## Collapse

This example shows how to collapse repeated log messages.

## What it demonstrates

- `LogmeD_Collapse(...)` / `LogmeD_CollapseIgnore(...)`
- `LogmeI_Collapse(...)` / `LogmeI_CollapseIgnore(...)`
- `LogmeW_Collapse(...)` / `LogmeW_CollapseIgnore(...)`
- `LogmeE_Collapse(...)` / `LogmeE_CollapseIgnore(...)`
- `LogmeC_Collapse(...)` / `LogmeC_CollapseIgnore(...)`
- `LogmeD_CollapseEvery(...)` / `LogmeD_CollapseIgnoreEvery(...)`
- `LogmeI_CollapseEvery(...)` / `LogmeI_CollapseIgnoreEvery(...)`
- `LogmeW_CollapseEvery(...)` / `LogmeW_CollapseIgnoreEvery(...)`
- `LogmeE_CollapseEvery(...)` / `LogmeE_CollapseIgnoreEvery(...)`
- `LogmeC_CollapseEvery(...)` / `LogmeC_CollapseIgnoreEvery(...)`

## Notes

- `LogmeX_Collapse(limit, ...)` uses the formatted message text as the repeat key.
- `LogmeX_CollapseIgnore(ignoreRegex, limit, ...)` removes all substrings matching `ignoreRegex` from the formatted message before comparing it with the previous message.
- `LogmeX_CollapseEvery(intervalMs, ...)` suppresses matching repeated messages until the interval expires.
- `LogmeX_CollapseIgnoreEvery(ignoreRegex, intervalMs, ...)` combines time-based collapse with ignored volatile substrings.
- `LogmeX_CollapseIgnore(...)` and `LogmeX_CollapseIgnoreEvery(...)` are intended for volatile fields such as request IDs, correlation IDs, timestamps, or counters.
- Ignore variants print the original formatted message. The regular expression is used only for comparison.
- `X` is one of the usual log levels: `D`, `I`, `W`, `E`, or `C`.
- The first message is printed immediately.
- Count-based collapse skips repeated messages from the same macro call site until the repeat counter reaches `limit`.
- Time-based collapse skips repeated messages from the same macro call site until `intervalMs` expires.
- Other log messages written by other source lines do not reset that call site's counter.
- The example intentionally writes a normal `LogmeI(...)` message between calls to `LogmeW_Collapse(...)` to show this behavior.
- When the count limit is reached or the time interval expires, logme prints one message with the repeat counter and then starts counting again.
- logme does not currently print a final summary for pending suppressed repeats when the log is closed.
- Time-based collapse does not print a pending summary when the repeat key changes. The new key starts a new collapse series.

## Repeated sequences and multi-line messages

Collapse works on one formatted message at one macro call site. It does not automatically detect that several different log records form a repeated block.

For a repeated sequence such as MQTT `Connecting`, `Connected`, `Subscribe`, and `Disconnected`, either collapse each log statement separately, or write one diagnostic message containing embedded `\n` characters if the whole sequence should be treated as one block.

For messages with changing ids, use `_CollapseIgnore` or `_CollapseIgnoreEvery` to remove the volatile part from the comparison key:

```cpp
LogmeI_CollapseIgnoreEvery(
  R"(\[mqtt [0-9]+\])"
  , 1000
  , "[mqtt %llu] Connecting to MQTT broker"
  , mqttId
);
```
