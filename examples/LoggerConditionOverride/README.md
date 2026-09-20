# LoggerConditionOverride example

## LoggerConditionOverride

This example shows how a class can replace the default precheck condition that every logging macro consults, scoped to that class's own methods only.

## What it demonstrates

- `LogmeD/LogmeI/LogmeW/LogmeE/...` call the unqualified, global `LoggerCondition()` instead of `Logme::Instance->Condition()` directly (a qualified `Logme::LoggerCondition()` call could not be shadowed by a class member).
- A class that declares its own non-static member named `LoggerCondition` hides the namespace-scope default via ordinary C++ member lookup, for any log macro invoked from that class's own methods.
- Unrelated code in the same program (a free function, or a class that does not declare the member) is unaffected and keeps using the process-wide default.
- The override is resolved at compile time (ordinary name lookup, no virtual dispatch) and costs nothing beyond whatever the override's own body does -- typically a single member read.

## Why it matters

`Logme::Instance->Condition()` is a single, process-wide callback (see `Logger::SetCondition`). That is the right granularity for a global on/off switch, but some call sites already know, once per object rather than once per process, whether logging is worth attempting -- for example a per-connection object that decided, when it was created, that a Sip/Dip/Host filter rules it out. Recomputing that decision on every log call site inside such an object is wasted work.

Declaring a `LoggerCondition()` member on that object's class lets every log macro called from its own methods reuse the cached decision directly, without changing behavior anywhere else in the program.

## Notes

- This mirrors the existing `CH`/`SUBSID` resolution mechanism (see `Logme/ID.h`, `Logme/SID.h`): a class member of the exact same name hides the file-scope default for that class's own methods.
- The override only affects code that is lexically a member function of a class declaring it. Free functions and other classes in the same file are never affected -- there is no macro-wide or namespace-wide side effect.
- See `tests/LoggerConditionOverride` for coverage of the override's independence from `Logger::SetCondition()`.
