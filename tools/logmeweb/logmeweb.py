import argparse
import asyncio
import hmac
import ipaddress
import secrets
import sys

if sys.version_info < (3, 9):
  print("logmeweb requires Python 3.9 or newer")
  sys.exit(1)

import tempfile
import threading
import time
from dataclasses import dataclass
from datetime import datetime
from datetime import timedelta
from datetime import timezone
from pathlib import Path
from typing import Optional

try:
  from fastapi import FastAPI
  from fastapi import HTTPException
  from fastapi import Request
  from fastapi.responses import FileResponse
  from fastapi.responses import HTMLResponse
  from fastapi.responses import JSONResponse
  from fastapi.responses import RedirectResponse
  from fastapi.staticfiles import StaticFiles
  from pydantic import BaseModel
  import uvicorn
except ImportError as e:
  print(f"Missing Python dependency: {e.name}")
  print("Install dependencies with: pip install -r tools/logmeweb/requirements.txt")
  sys.exit(1)

from control_client import SendControlCommand
from discovery import DiscoverProcesses


BASE_DIR = Path(__file__).resolve().parent
SESSION_COOKIE_NAME = "logmeweb_session"

app = FastAPI(title="logmeweb")
app.mount("/static", StaticFiles(directory=str(BASE_DIR / "static")), name="static")


class CommandRequest(BaseModel):
  host: str
  port: int
  protocol: str = "http"
  password: Optional[str] = ""
  command: str
  format: str = "text"


class LoginRequest(BaseModel):
  password: str = ""


@dataclass
class Session:
  client_ip: str
  user_agent: str
  created_at: float
  last_seen: float


@dataclass
class LoginBlock:
  failures: int = 0
  blocked_until: float = 0.0


class AuthManager:
  def __init__(self):
    self.password = ""
    self.enabled = False
    self.session_timeout_seconds = 8 * 60 * 60
    self.max_failures = 5
    self.base_block_seconds = 60
    self.max_block_seconds = 5 * 60
    self.secure_cookie = False
    self.sessions = {}
    self.blocks = {}

  def Configure(self, args):
    self.password = args.password or ""
    self.enabled = bool(self.password)
    self.session_timeout_seconds = max(60, int(args.session_timeout_minutes * 60))
    self.max_failures = max(1, int(args.login_max_failures))
    self.base_block_seconds = max(1, int(args.login_block_seconds))
    self.max_block_seconds = max(self.base_block_seconds, int(args.login_max_block_seconds))
    self.secure_cookie = bool(args.https)
    self.trust_proxy_headers = bool(args.trust_proxy_headers)

  def GetForwardedClientIp(self, request):
    single_ip_headers = (
      "cf-connecting-ip"
      , "true-client-ip"
      , "x-real-ip"
    )

    for header_name in single_ip_headers:
      client_ip = request.headers.get(header_name, "").strip()
      if client_ip:
        return client_ip

    forwarded_for = request.headers.get("x-forwarded-for", "").strip()
    if forwarded_for:
      return forwarded_for.split(",", 1)[0].strip()

    forwarded = request.headers.get("forwarded", "")
    for part in forwarded.split(";"):
      name, separator, value = part.strip().partition("=")
      if separator and name.lower() == "for":
        return value.strip().strip("\"").strip("[]")

    return ""

  def GetClientIp(self, request):
    if self.trust_proxy_headers:
      forwarded_ip = self.GetForwardedClientIp(request)
      if forwarded_ip:
        return forwarded_ip

    if not request.client:
      return ""

    return request.client.host or ""

  def GetUserAgent(self, request):
    return request.headers.get("user-agent", "")

  def IsPublicPath(self, path):
    if path == "/login" or path == "/api/login" or path == "/api/logout":
      return True

    if path.startswith("/static/"):
      return True

    if path == "/favicon.ico":
      return True

    return False

  def IsAuthenticated(self, request):
    if not self.enabled:
      return True

    session_id = request.cookies.get(SESSION_COOKIE_NAME)
    if not session_id:
      return False

    session = self.sessions.get(session_id)
    if not session:
      return False

    now = time.monotonic()
    if (now - session.last_seen) > self.session_timeout_seconds:
      self.sessions.pop(session_id, None)
      return False

    if session.client_ip != self.GetClientIp(request):
      self.sessions.pop(session_id, None)
      return False

    if session.user_agent != self.GetUserAgent(request):
      self.sessions.pop(session_id, None)
      return False

    session.last_seen = now
    return True

  def GetBlock(self, request):
    client_ip = self.GetClientIp(request)
    return self.blocks.setdefault(client_ip, LoginBlock())

  def CheckBlocked(self, request):
    block = self.GetBlock(request)
    now = time.monotonic()
    if block.blocked_until <= now:
      return 0

    return int(block.blocked_until - now) + 1

  def RegisterFailure(self, request):
    block = self.GetBlock(request)
    block.failures += 1

    if block.failures < self.max_failures:
      return 0

    exponent = max(0, block.failures - self.max_failures)
    delay = min(self.max_block_seconds, self.base_block_seconds * (2 ** exponent))
    block.blocked_until = time.monotonic() + delay
    return delay

  def RegisterSuccess(self, request):
    client_ip = self.GetClientIp(request)
    self.blocks.pop(client_ip, None)

    now = time.monotonic()
    session_id = secrets.token_urlsafe(32)
    self.sessions[session_id] = Session(
      client_ip=client_ip
      , user_agent=self.GetUserAgent(request)
      , created_at=now
      , last_seen=now
    )
    return session_id

  def Login(self, request, password):
    blocked_seconds = self.CheckBlocked(request)
    if blocked_seconds > 0:
      return None, blocked_seconds

    if not hmac.compare_digest(password or "", self.password):
      self.RegisterFailure(request)
      return None, 0

    return self.RegisterSuccess(request), 0

  def Logout(self, request):
    session_id = request.cookies.get(SESSION_COOKIE_NAME)
    if session_id:
      self.sessions.pop(session_id, None)


Auth = AuthManager()


@app.middleware("http")
async def AuthMiddleware(request: Request, call_next):
  if not Auth.enabled or Auth.IsPublicPath(request.url.path):
    return await call_next(request)

  if Auth.IsAuthenticated(request):
    return await call_next(request)

  if request.url.path.startswith("/api/"):
    return JSONResponse({"detail": "Authentication required"}, status_code=401)

  return RedirectResponse(url="/login", status_code=302)


@app.get("/")
async def Index():
  html = (BASE_DIR / "templates" / "index.html").read_text(encoding="utf-8")
  auth_class = "auth-enabled" if Auth.enabled else "auth-disabled"
  html = html.replace("<body>", f"<body class=\"{auth_class}\">")
  return HTMLResponse(html)


@app.get("/login")
async def LoginPage(request: Request):
  if not Auth.enabled:
    return RedirectResponse(url="/", status_code=302)

  if Auth.IsAuthenticated(request):
    return RedirectResponse(url="/", status_code=302)

  return FileResponse(str(BASE_DIR / "templates" / "login.html"))


@app.post("/api/login")
async def ApiLogin(request: Request, login: LoginRequest):
  if not Auth.enabled:
    return {"ok": True}

  session_id, blocked_seconds = Auth.Login(request, login.password)
  if blocked_seconds > 0:
    raise HTTPException(
      status_code=429
      , detail=f"Too many login attempts. Try again in {blocked_seconds} second(s)."
    )

  if not session_id:
    raise HTTPException(status_code=401, detail="Login failed")

  response = JSONResponse({"ok": True})
  response.set_cookie(
    SESSION_COOKIE_NAME
    , session_id
    , max_age=Auth.session_timeout_seconds
    , httponly=True
    , secure=Auth.secure_cookie
    , samesite="lax"
  )
  return response


@app.post("/api/logout")
async def ApiLogout(request: Request):
  Auth.Logout(request)
  response = JSONResponse({"ok": True})
  response.delete_cookie(SESSION_COOKIE_NAME)
  return response


@app.get("/api/discovery")
async def ApiDiscovery():
  return DiscoverProcesses()


@app.post("/api/command")
async def ApiCommand(request: CommandRequest):
  try:
    response = SendControlCommand(
      request.host
      , request.port
      , request.protocol
      , request.password or ""
      , request.command
      , request.format
    )
    return JSONResponse(
      response
      , media_type="application/json; charset=utf-8"
    )
  except Exception as e:
    raise HTTPException(status_code=500, detail=str(e))


def AddHostToSan(host, san_items):
  try:
    ip = ipaddress.ip_address(host)
    if ip.is_unspecified:
      return False

    san_items.append(ip)
    return True
  except ValueError:
    if host and host not in ("localhost", "0.0.0.0", "::"):
      san_items.append(host)
      return True

  return False


def GenerateSelfSignedCertificate(host):
  try:
    from cryptography import x509
    from cryptography.hazmat.primitives import hashes
    from cryptography.hazmat.primitives import serialization
    from cryptography.hazmat.primitives.asymmetric import rsa
    from cryptography.x509.oid import NameOID
  except ImportError as e:
    raise RuntimeError(
      "Automatic HTTPS certificate generation requires the cryptography package. "
      "Install dependencies with: python -m pip install -r tools/logmeweb/requirements.txt"
    ) from e

  key = rsa.generate_private_key(
    public_exponent=65537
    , key_size=2048
  )

  subject = issuer = x509.Name([
    x509.NameAttribute(NameOID.COMMON_NAME, "localhost")
  ])

  san_values = [
    "localhost"
    , ipaddress.ip_address("127.0.0.1")
    , ipaddress.ip_address("::1")
  ]
  host_added = AddHostToSan(host, san_values)
  san_items = []

  for value in san_values:
    if isinstance(value, str):
      san_items.append(x509.DNSName(value))
    else:
      san_items.append(x509.IPAddress(value))

  now = datetime.now(timezone.utc)
  certificate = (
    x509.CertificateBuilder()
      .subject_name(subject)
      .issuer_name(issuer)
      .public_key(key.public_key())
      .serial_number(x509.random_serial_number())
      .not_valid_before(now - timedelta(minutes=1))
      .not_valid_after(now + timedelta(days=365))
      .add_extension(
        x509.SubjectAlternativeName(san_items)
        , critical=False
      )
      .sign(key, hashes.SHA256())
  )

  temp_dir = tempfile.TemporaryDirectory(prefix="logmeweb_https_")
  cert_path = Path(temp_dir.name) / "logmeweb-cert.pem"
  key_path = Path(temp_dir.name) / "logmeweb-key.pem"

  cert_path.write_bytes(certificate.public_bytes(serialization.Encoding.PEM))
  key_path.write_bytes(key.private_bytes(
    encoding=serialization.Encoding.PEM
    , format=serialization.PrivateFormat.TraditionalOpenSSL
    , encryption_algorithm=serialization.NoEncryption()
  ))

  return temp_dir, str(cert_path), str(key_path), host_added


def ParseArgs():
  parser = argparse.ArgumentParser(description="logme web control UI")
  parser.add_argument("--host", default="127.0.0.1", help="Web UI host address.")
  parser.add_argument("--port", type=int, default=8080, help="Web UI port.")
  parser.add_argument("--https", action="store_true", help="Serve the web UI over HTTPS.")
  parser.add_argument("--cert", help="HTTPS certificate PEM file.")
  parser.add_argument("--key", help="HTTPS private key PEM file.")
  parser.add_argument("--password", default="", help="Require this password for web UI access.")
  parser.add_argument("--session-timeout-minutes", type=int, default=480, help="Authenticated web session timeout.")
  parser.add_argument("--login-max-failures", type=int, default=5, help="Failed login attempts before temporary block.")
  parser.add_argument("--login-block-seconds", type=int, default=60, help="Initial temporary block duration after too many failed logins.")
  parser.add_argument("--login-max-block-seconds", type=int, default=300, help="Maximum temporary block duration after repeated failed logins.")
  parser.add_argument(
    "--trust-proxy-headers"
    , action="store_true"
    , help="Trust reverse proxy client IP headers such as X-Forwarded-For, X-Real-IP, Forwarded, and CF-Connecting-IP."
  )
  return parser.parse_args()


def ResolveHttpsFiles(args):
  if not args.https:
    if args.cert or args.key:
      raise ValueError("--cert and --key require --https.")

    return None, None, None

  if bool(args.cert) != bool(args.key):
    raise ValueError("--cert and --key must be specified together.")

  if args.cert and args.key:
    cert_path = Path(args.cert)
    key_path = Path(args.key)

    if not cert_path.is_file():
      raise FileNotFoundError(f"Certificate file not found: {cert_path}")

    if not key_path.is_file():
      raise FileNotFoundError(f"Private key file not found: {key_path}")

    return None, str(cert_path), str(key_path)

  temp_dir, cert_path, key_path, host_added = GenerateSelfSignedCertificate(args.host)
  print("Generated temporary self-signed HTTPS certificate for localhost and 127.0.0.1.")

  if host_added:
    print(f"Added {args.host} to the temporary HTTPS certificate SAN list.")
  elif args.host in ("0.0.0.0", "::"):
    print("Warning: --host uses an unspecified address. Use --cert and --key for external HTTPS access.")

  return temp_dir, cert_path, key_path


def SetupEventLoopPolicy():
  if sys.platform != "win32":
    return

  if not hasattr(asyncio, "WindowsSelectorEventLoopPolicy"):
    return

  asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())


def RunServer(args, cert_path, key_path):
  Auth.Configure(args)

  config_args = {
    "host": args.host,
    "port": args.port,
    "reload": False,
    "ssl_certfile": cert_path,
    "ssl_keyfile": key_path,
    "timeout_keep_alive": 1,
    "timeout_graceful_shutdown": 1,
  }

  try:
    config = uvicorn.Config(app, **config_args)
  except TypeError as e:
    if "timeout_graceful_shutdown" not in str(e):
      raise

    config_args.pop("timeout_graceful_shutdown", None)
    config = uvicorn.Config(app, **config_args)
  server = uvicorn.Server(config)
  error = []

  def ServerThread():
    try:
      server.run()
    except BaseException as e:
      error.append(e)

  thread = threading.Thread(target=ServerThread, name="logmeweb-server", daemon=True)
  thread.start()

  start_time = time.monotonic()
  while thread.is_alive() and not server.started and not error:
    if (time.monotonic() - start_time) > 5.0:
      break

    time.sleep(0.05)

  if error:
    raise error[0]

  return server, thread, error


def StopServer(server, thread):
  server.should_exit = True
  thread.join(timeout=3.0)

  if thread.is_alive():
    server.force_exit = True
    thread.join(timeout=1.0)


def WaitForStop(server, thread, error):
  try:
    input("Press Enter to stop logmeweb.\n")
  except KeyboardInterrupt:
    print()
  finally:
    StopServer(server, thread)

  if error:
    raise error[0]


if __name__ == "__main__":
  SetupEventLoopPolicy()
  args = ParseArgs()
  temp_dir, cert_path, key_path = ResolveHttpsFiles(args)
  scheme = "https" if args.https else "http"
  print(f"Open logmeweb at: {scheme}://{args.host}:{args.port}")

  if args.password:
    print("Web UI authentication is enabled.")

  server, thread, error = RunServer(args, cert_path, key_path)
  WaitForStop(server, thread, error)
