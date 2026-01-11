# Control API

This document describes the **logme control interface** used by `logmectl` and the embedded control server.

The API supports two output formats:

- `text` (default): human-readable text, printed as-is.
- `json`: machine-readable JSON envelope.

## CLI

`logmectl` supports:

- `--format text|json` (default: `text`)

Example:

```text
logmectl -p 7791 subsystem
logmectl -p 7791 --format json subsystem
```

## Text format

In `--format text` mode, the client prints the server response **as-is**.

Errors are indicated by responses that start with:

```text
error: <message>
```

The server guarantees that a response is always returned (never empty).
For “empty” results the text explicitly states `none` rather than printing an empty section.

## JSON format

In `--format json` mode, the client sends the command to the server wrapped as:

```text
format json <command...>
```

The server replies with a single JSON object:

```json
{
  "ok": true,
  "error": null,
  "data": {}
}
```

### Envelope

- `ok` (boolean)  
  `true` on success, `false` on error.

- `error` (string|null)  
  `null` on success, otherwise a human-readable error message.

- `data` (object)  
  Command-specific result data. Always present.

Notes:

- In JSON mode, errors are represented only by `ok=false` and `error`.
  The server does **not** use a `error:` prefix inside JSON.
- `data` may include an optional `text` field (string) which contains the same human-readable output as the text format.
  This is intended for debugging and backward compatibility.

## Command schemas

Only fields documented below are guaranteed. New fields may be added in the future.

### `subsystem`

Text example:

```text
BlockReportedSubsystems: true
Reported subsystems: none
```

JSON:

```json
{
  "ok": true,
  "error": null,
  "data": {
    "blockReportedSubsystems": true,
    "reportedSubsystems": ["core", "net"]
  }
}
```

Fields:

- `blockReportedSubsystems` (bool)
- `reportedSubsystems` (array of strings)

When no subsystems are reported, `reportedSubsystems` is `[]`.

### `list`

Text example:

```text
<default>
net
ssl
```

JSON:

```json
{
  "ok": true,
  "error": null,
  "data": {
    "channels": ["", "net", "ssl"]
  }
}
```

Fields:

- `channels` (array of strings)  
  The default channel is represented as an empty string `""`.

### `flags`

Text example:

```text
Flags: 0x00000001 timestamps
```

JSON:

```json
{
  "ok": true,
  "error": null,
  "data": {
    "value": 1,
    "names": ["timestamps"]
  }
}
```

Fields:

- `value` (integer)  
  Raw flags bitmask.
- `names` (array of strings)  
  Flag names (space-separated tokens from the textual representation).

### `level`

Text example:

```text
Level: info
```

JSON:

```json
{
  "ok": true,
  "error": null,
  "data": {
    "level": "info"
  }
}
```

Fields:

- `level` (string)

### `backend`, `channel`, `help`

These commands are supported in JSON mode and return the standard envelope.
For operations that only report success, `data` may be `{}`.

In the future these commands may return structured `data` objects as needed.
