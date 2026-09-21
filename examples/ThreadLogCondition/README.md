# ThreadLogCondition example

## ThreadLogCondition

This example shows how to gate every logging macro call for a stretch of work on the current thread, for code that has no per-object state of its own to cache a `LoggerCondition()` member override on (see `examples/LoggerConditionOverride`).

## What it demonstrates

- `LogmeThreadCondition(condition)` adds `condition` to the default `LoggerCondition()`'s answer for the current thread, from construction until the guard goes out of scope -- mirroring `LogmeThreadChannel`/`LogmeThreadSubsystem`.
- A shared, stateless object (here, `RuleNode`, standing in for a parsed rule/DSL tree walked on behalf of many different requests) benefits without needing any member of its own: the decision lives on the thread, not the object.
- `LogmeThreadCondition` scopes nest and restore correctly, exactly like `LogmeThreadChannel`/`LogmeThreadSubsystem`.
- A class that already declares its own `LoggerCondition()` member (see `examples/LoggerConditionOverride`) is completely unaffected by `LogmeThreadCondition` -- ordinary C++ member lookup means its override hides the global default (thread condition included) entirely, at zero extra cost.

## Why it matters

The `LoggerCondition()` member-override trick assumes the object making the log call has somewhere to cache a per-request answer. That is true for a per-connection or per-request object, but not for a shared, cached structure evaluated on behalf of whichever request happens to be running right now (a parsed policy/DSL rule tree is the motivating case). Such code can still know, at the single point it starts evaluating on the current thread, that the whole stretch of work about to happen can skip logging entirely -- `LogmeThreadCondition` lets it say so once instead of touching every node's log call site.

## Notes

- Declared outside `namespace Logme` the same way `LoggerCondition()` is (see `Logme/Logger.h`) is not required here: `LogmeThreadCondition` is a macro that constructs a `Logme::ThreadCondition` RAII guard, unrelated to the member-lookup shadowing trick.
- The global default `LoggerCondition()` checks `Logme::Instance->Condition() && Logme::Instance->GetThreadLogCondition()` -- a class's own override still fully replaces this, so the thread condition is only ever consulted for code that does not opt into its own override.
- See `tests/ThreadLogCondition` for coverage of nesting/restore behavior and independence from `LoggerCondition()` member overrides.
