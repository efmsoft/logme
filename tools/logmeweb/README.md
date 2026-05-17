# logmeweb


## Requirements

- Python 3.9 or newer
- fastapi
- uvicorn

Install dependencies:

```bash
pip install -r tools/logmeweb/requirements.txt
```

Check Python version:

```bash
python3 --version
```

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

The web UI host and port can be changed when needed. Use `0.0.0.0` to listen on all interfaces:

```bash
python tools/logmeweb/logmeweb.py --host 127.0.0.1 --port 8080
python tools/logmeweb/logmeweb.py --host 0.0.0.0 --port 8080
python tools/logmeweb/logmeweb.py --host 192.168.1.10 --port 8080
```

## HTTPS

`logmeweb` can serve the UI over HTTPS:

```bash
python tools/logmeweb/logmeweb.py --https
```

When `--https` is used without `--cert` and `--key`, `logmeweb` generates a temporary self-signed certificate automatically. The generated certificate is valid for `localhost`, `127.0.0.1`, and `::1` and is removed when the process exits. Browsers will still show a warning because the certificate is self-signed.

When an explicit IP address is passed through `--host`, the automatically generated certificate also includes that IP address in the SAN list. For `--host 0.0.0.0` or `--host ::`, use an explicit certificate because the browser connects to a concrete address, not to the unspecified bind address.

A custom certificate can be provided explicitly:

```bash
python tools/logmeweb/logmeweb.py --https --cert server-cert.pem --key server-key.pem
```


## Web UI authentication

By default, `logmeweb` keeps the previous local-only behavior and does not require a web UI password. When `--password` is specified, the root page is protected by a login screen:

```bash
python tools/logmeweb/logmeweb.py --host 0.0.0.0 --port 8080 --https --password 1234
```

After a successful login, the server creates an in-memory session and sends a session cookie. The cookie is `HttpOnly`, uses `SameSite=Lax`, and is marked `Secure` when HTTPS is enabled. The cookie contains only a random session id, not the password. A session is bound to the client IP address and User-Agent and expires after the configured inactivity timeout. When `logmeweb` is accessed through Cloudflare, the client IP is taken from the `CF-Connecting-IP` header so that session validation keeps working even if Cloudflare uses different edge addresses for subsequent requests.

Useful authentication options:

```bash
python tools/logmeweb/logmeweb.py --password 1234 --session-timeout-minutes 480
python tools/logmeweb/logmeweb.py --password 1234 --login-max-failures 5 --login-block-seconds 60 --login-max-block-seconds 300
```

Repeated failed logins from the same client IP are temporarily blocked. Error messages intentionally do not reveal whether the password was close or valid.

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


## Reverse proxy / CDN deployment

When logmeweb is accessed through a reverse proxy or CDN, the TCP peer address seen by
the application may be the proxy address rather than the real browser address. This can
break session validation because authenticated sessions are bound to the client identity.

Use `--trust-proxy-headers` only when logmeweb is behind a proxy that controls and
overwrites the forwarded client IP headers:

```bash
python3 logmeweb.py \
  --host 0.0.0.0 \
  --port 2053 \
  --https \
  --cert /path/to/fullchain.pem \
  --key /path/to/privkey.pem \
  --password "your-password" \
  --trust-proxy-headers
```

With this option enabled, logmeweb uses these headers to determine the client address:

- `CF-Connecting-IP`
- `True-Client-IP`
- `X-Real-IP`
- the first address from `X-Forwarded-For`
- `Forwarded: for=...`

Do not enable this option for direct public access unless a trusted proxy strips or
replaces these headers, because clients can otherwise spoof them.
