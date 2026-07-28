# Subsystem filtering and level overrides

A subsystem is an optional named classification attached to a log record inside
a channel. It can be used for two independent purposes:

- allow/block filtering;
- an optional level override for a named subsystem.

These mechanisms are global to a `Logger`, while the fallback level remains a
property of each channel.

## Effective filtering rules

For every channel receiving a record, filtering is applied in this order:

1. An inactive channel rejects the record.
2. A blocked subsystem rejects the record.
3. If the allowed list is not empty, an unlisted named subsystem is rejected.
4. If the named subsystem has a level override, that level is used.
5. Otherwise, the current channel level is used.

A subsystem level **replaces** the channel level. It is not combined with the
channel level through a minimum or maximum operation.

For a channel configured at `INFO`:

| Subsystem configuration | `DEBUG` record | `INFO` record | `WARN` record |
|---|---:|---:|---:|
| no override | rejected | logged | logged |
| `DSL = DEBUG` | logged | logged | logged |
| `CLOUD = WARN` | rejected | rejected | logged |

A linked or fan-out record is checked independently by every destination
channel. When no subsystem override exists, each destination uses its own
channel level. When an override exists, the same subsystem level replaces the
level of each destination channel.

Messages without a named subsystem always use the channel level. A level cannot
be assigned to the empty default `SUBSID`.

## JSON configuration

Define level overrides inside `subsystems.levels`:

```json
{
  "subsystems": {
    "blocked": ["NOISE"],
    "allowed": ["DSL", "CLOUD"],
    "levels": {
      "DSL": "DEBUG",
      "CLOUD": "WARN"
    }
  }
}
```

`levels` must be a JSON object. Every key must be a non-empty subsystem name and
every value must be a supported level name. Level names use the same parser as
channel levels and are case-insensitive.

Loading a configuration replaces the existing subsystem configuration. Old
blocked, allowed, and level entries are cleared first. Therefore, omitting
`subsystems.levels` from a later configuration clears previously configured
level overrides.

## C++ API

The runtime API is available through `Logger`:

```cpp
#include <Logme/Logme.h>

using namespace Logme;

void ConfigureSubsystemLevels()
{
  Instance->SetSubsystemLevel(SID::Build("DSL"), LEVEL_DEBUG);
  Instance->SetSubsystemLevel(SID::Build("CLOUD"), LEVEL_WARN);

  Level level = LEVEL_INFO;
  if (Instance->GetSubsystemLevel(SID::Build("DSL"), level))
  {
    // level is LEVEL_DEBUG
  }

  Instance->RemoveSubsystemLevel(SID::Build("CLOUD"));
  Instance->ClearSubsystemLevels();
}
```

Available operations:

```cpp
void SetSubsystemLevel(const SID& sid, Level level);
bool GetSubsystemLevel(const SID& sid, Level& level);
void RemoveSubsystemLevel(const SID& sid);
void ClearSubsystemLevels();
```

`SetSubsystemLevel()` replaces an existing override for the same subsystem.
Empty subsystem identifiers are ignored. Configuration may be changed while
logging is active.

Allow/block filters remain separate:

```cpp
Instance->AddBlockedSubsystem(SID::Build("NOISE"));
Instance->AddAllowedSubsystem(SID::Build("DSL"));
```

A blocked subsystem is never restored by a level override.

## Runtime control

The built-in control server exposes the same functionality:

```text
subsystem --set-level DSL DEBUG
subsystem --set-level CLOUD WARN
subsystem --check DSL
subsystem --remove-level CLOUD
subsystem --clear-levels
```

List all subsystem settings:

```text
subsystem
```

Example output:

```text
Blocked subsystems: none
Allowed subsystems: none
Subsystem levels:
  CLOUD: WARN
  DSL: DEBUG
```

`subsystem --clear` clears blocked, allowed, and level-override entries. See
[Control API](control_api.md) for the text and JSON response schemas.

## Thread subsystem behavior

A thread subsystem participates in the same final filtering rules as a subsystem
selected directly by the log call. Calling `SetThreadSubsystem()` with the
empty default `SUBSID` clears the thread subsystem.

The logging precheck remains conservative when subsystem level overrides are
configured. If a named call-site subsystem or a named thread subsystem may
change the effective level, the final decision is deferred until the complete
record context is available. When no subsystem levels are configured, the
existing channel-only fast path is retained.

## Performance model

Per-subsystem levels are intended as a precise diagnostic control and are
expected to be configured for a small number of subsystems.

When no level overrides exist, the hot path avoids table lookup and locking.
When overrides exist, only records with a named subsystem require the final
lookup. The logger atomically publishes an immutable, sorted snapshot of the
configured levels. Readers perform one atomic pointer load and a lock-free
lookup; runtime updates allocate and publish a new snapshot under the logger
lock. Previous snapshots remain valid until the logger is destroyed, so readers
never require reference counting or reclamation synchronization.
