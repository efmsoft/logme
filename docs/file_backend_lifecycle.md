# File backend lifecycle policies

`FileBackend` supports file-size control, time-based rotation, archive naming, retention, and optional gzip compression. The hot write path stays focused on appending data; directory scans, archive-name recovery, retention cleanup, and compression are done only when a file is completed or when startup cleanup is explicitly enabled.

## Minimal file backend

```json
{
  "type": "FileBackend",
  "file": "logs/app.log",
  "append": true
}
```

## Size limit behavior

`max-size` limits the active file size. By default, logme keeps the historical behavior and truncates the active file when the limit is exceeded.

```json
{
  "type": "FileBackend",
  "file": "logs/app.log",
  "max-size": "100Mb",
  "on-size-limit": "truncate"
}
```

To rotate instead of truncating, set `on-size-limit` to `rotate` and provide an `archive` pattern containing `{index}`:

```json
{
  "type": "FileBackend",
  "file": "logs/app.log",
  "max-size": "100Mb",
  "on-size-limit": "rotate",
  "archive": "logs/archive/app.{index}.log"
}
```

If `on-size-limit` is `rotate`, `archive` is required and must contain `{index}`. Existing `.log` and `.log.gz` archive names are skipped, so restart recovery and runtime collision handling do not overwrite existing archives.

## Time rotation

The `rotation` option accepts:

- `none`, `off`, or `disabled`
- `hourly`
- `daily`
- `weekly`
- `monthly`

Example:

```json
{
  "type": "FileBackend",
  "file": "logs/app.log",
  "rotation": "daily",
  "archive": "logs/archive/app.{date}.{index}.log"
}
```

`daily` preserves the old daily-rotation configuration path. If time rotation is disabled, the hot path does not compute time-period boundaries.

## Combined time and size rotation

Size and time rotation can be used together. Both use the same file-completion path.

```json
{
  "type": "FileBackend",
  "file": "logs/app.log",
  "rotation": "daily",
  "max-size": "100Mb",
  "on-size-limit": "rotate",
  "archive": "logs/archive/app.{date}.{index}.log"
}
```

The archive pattern can use `{index}`, `{date}`, and `{datetime}`. With `{date}` or `{datetime}` plus `{index}`, the index is recovered for the current time period. Archives from other periods do not consume the current period index.

## Retention

Retention rules apply to completed archive files, not to the active file. `max-parts` remains available as the legacy alias for `retention.max-files`. If both are specified, they must have the same value.

```json
{
  "type": "FileBackend",
  "file": "logs/app.log",
  "rotation": "daily",
  "archive": "logs/archive/app.{date}.{index}.log",
  "retention": {
    "max-files": 14,
    "max-age": "30d",
    "max-total-size": "2Gb",
    "clean-on-start": true
  }
}
```

Disabled values:

- `retention.max-files = 0` keeps all matching archive files;
- `retention.max-age = 0` disables age-based cleanup;
- `retention.max-total-size = 0` disables total-size cleanup.

`clean-on-start` runs retention after configuration is applied. Runtime retention also runs after file completion.

## Compression

Optional gzip compression is enabled with:

```json
{
  "type": "FileBackend",
  "file": "logs/app.log",
  "rotation": "daily",
  "archive": "logs/archive/app.{date}.{index}.log",
  "compression": "gz"
}
```

Accepted enabled values are `gz` and `gzip`. Disabled values are `none`, `off`, `disabled`, or an empty string. Gzip support depends on the `USE_ZLIB` CMake option. If zlib support is not compiled in, `compression: "gz"` is accepted but has no effect.

Compression is submitted only for completed archive files. The active file is not compressed.

## Full production-style example

```json
{
  "type": "FileBackend",
  "file": "logs/app.log",
  "append": true,
  "rotation": "daily",
  "max-size": "100Mb",
  "on-size-limit": "rotate",
  "archive": "logs/archive/app.{date}.{index}.log",
  "compression": "gz",
  "retention": {
    "max-files": 14,
    "max-age": "30d",
    "max-total-size": "2Gb",
    "clean-on-start": true
  }
}
```

## Failure behavior

File rotation is best-effort. If an archive directory cannot be created or the active file cannot be moved to an archive name, the backend reopens the active file in append mode and keeps logging when possible. This avoids losing existing active-log content on rotation failure.
