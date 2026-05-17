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
Blocked subsystems: none
Allowed subsystems: none
```

JSON:

```json
{
  "ok": true,
  "error": null,
  "data": {
    "blockedSubsystems": ["noise"],
    "allowedSubsystems": ["core", "net"]
  }
}
```

Fields:

- `blockedSubsystems` (array of strings)
- `allowedSubsystems` (array of strings)

When no subsystems are blocked or allowed, the corresponding array is `[]`.

The blocked list has priority. When the allowed list is not empty, only listed
subsystems are logged, unless they are also blocked. Messages without a
subsystem are not affected by subsystem filtering.

Supported text commands:

```text
subsystem
subsystem --block name
subsystem --unblock name
subsystem --allow name
subsystem --disallow name
subsystem --clear-blocked
subsystem --clear-allowed
subsystem --clear
subsystem --check name
```

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


### `logs`

The `logs` command exposes a read-only view of log files below the current
logger home directory.

Supported text commands:

```text
logs --info
logs --tree [relative-path]
logs --tail relative-file-path [bytes]
logs --read relative-file-path [offset] [bytes]
logs --download relative-file-path
```

The command never accepts absolute paths and rejects paths that resolve outside
the logger home directory. Only files with extensions configured in
`home-directory.watch-dog.file-extension` are exposed. If the configured list is
empty, the control command uses the standard log extensions:

```text
.log .nlb .nlr .b64 .dat .csv
```

`logs --tree` returns tab-separated lines:

```text
Home directory: /var/log/my-app/
Path: logs
DIR     logs/archive
FILE    logs/app.log  123456  132456789
```

`logs --tail` returns the last part of the selected file. The optional `bytes`
argument is capped by the server to avoid returning very large files in one
response.

`logs --read` returns a bounded range of the selected file. The response starts
with a metadata header followed by the file chunk:

```text
LOGMEWEB-RANGE    offset    requested-bytes    file-size
```

`logs --download` returns the selected file encoded as base64 with a metadata
header:

```text
LOGMEWEB-DOWNLOAD-B64    file-size
```

The download response is intended for `logmeweb` and is also capped by the
server to avoid transferring unexpectedly huge files through the control
interface.
