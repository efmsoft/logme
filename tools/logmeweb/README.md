# logmeweb

`logmeweb` is a lightweight web UI for the logme control server.

It does not use React, npm, or a frontend build step. The tool can be started directly from the repository.

## Run

```bash
python -m pip install -r tools/logmeweb/requirements.txt
python tools/logmeweb/logmeweb.py
```

Open:

```text
http://127.0.0.1:8080
```

## Local discovery

Discovery is opt-in on the logme control server side.

When enabled, the running process exposes only connection metadata:

```json
{
  "version": 1,
  "pid": 12345,
  "process": "my-app",
  "control": {
    "host": "127.0.0.1",
    "port": 19090,
    "protocol": "http"
  },
  "authRequired": true
}
```

Passwords are never exposed through discovery. The user enters the password in the web UI.

## Platform discovery endpoints

- Windows: named pipe `\\.\pipe\logme-discovery-<pid>`
- Linux: abstract Unix domain socket `logme-discovery-<pid>`
- macOS/other Unix: Unix domain socket `/tmp/logme-discovery-<pid>.sock`
