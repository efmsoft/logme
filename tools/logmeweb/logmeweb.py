import argparse
import asyncio
import ipaddress
import sys
import tempfile
import threading
import time
from datetime import datetime
from datetime import timedelta
from datetime import timezone
from pathlib import Path
from typing import Optional

from fastapi import FastAPI
from fastapi import HTTPException
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel
import uvicorn

from control_client import SendControlCommand
from discovery import DiscoverProcesses


BASE_DIR = Path(__file__).resolve().parent

app = FastAPI(title="logmeweb")
app.mount("/static", StaticFiles(directory=str(BASE_DIR / "static")), name="static")


class CommandRequest(BaseModel):
  host: str
  port: int
  protocol: str = "http"
  password: Optional[str] = ""
  command: str
  format: str = "text"


@app.get("/")
async def Index():
  return FileResponse(str(BASE_DIR / "templates" / "index.html"))


@app.get("/api/discovery")
async def ApiDiscovery():
  return DiscoverProcesses()


@app.post("/api/command")
async def ApiCommand(request: CommandRequest):
  try:
    return SendControlCommand(
      request.host
      , request.port
      , request.protocol
      , request.password or ""
      , request.command
      , request.format
    )
  except Exception as e:
    raise HTTPException(status_code=500, detail=str(e))


def GenerateSelfSignedCertificate():
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
        x509.SubjectAlternativeName([
          x509.DNSName("localhost")
          , x509.IPAddress(ipaddress.ip_address("127.0.0.1"))
          , x509.IPAddress(ipaddress.ip_address("::1"))
        ])
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

  return temp_dir, str(cert_path), str(key_path)


def ParseArgs():
  parser = argparse.ArgumentParser(description="logme web control UI")
  parser.add_argument("--host", default="127.0.0.1", help="Web UI host address.")
  parser.add_argument("--port", type=int, default=8080, help="Web UI port.")
  parser.add_argument("--https", action="store_true", help="Serve the web UI over HTTPS.")
  parser.add_argument("--cert", help="HTTPS certificate PEM file.")
  parser.add_argument("--key", help="HTTPS private key PEM file.")
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

  temp_dir, cert_path, key_path = GenerateSelfSignedCertificate()
  print("Generated temporary self-signed HTTPS certificate for localhost and 127.0.0.1.")
  return temp_dir, cert_path, key_path


def SetupEventLoopPolicy():
  if sys.platform != "win32":
    return

  if not hasattr(asyncio, "WindowsSelectorEventLoopPolicy"):
    return

  asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())


def RunServer(args, cert_path, key_path):
  config = uvicorn.Config(
    "logmeweb:app"
    , host=args.host
    , port=args.port
    , reload=False
    , ssl_certfile=cert_path
    , ssl_keyfile=key_path
    , timeout_keep_alive=1
    , timeout_graceful_shutdown=1
  )
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

  server, thread, error = RunServer(args, cert_path, key_path)
  WaitForStop(server, thread, error)
