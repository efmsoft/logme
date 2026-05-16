import json
import socket
import ssl


RECV_FIRST_TIMEOUT = 5.0
RECV_DRAIN_TIMEOUT = 0.15


def _recv_response(sock):
  data = bytearray()
  got_first = False

  while True:
    sock.settimeout(RECV_FIRST_TIMEOUT if not got_first else RECV_DRAIN_TIMEOUT)

    try:
      chunk = sock.recv(4096)
    except TimeoutError:
      break
    except socket.timeout:
      break

    if not chunk:
      break

    data.extend(chunk)
    got_first = True

  return data.decode("utf-8", errors="replace")


def _send_request(sock, text):
  sock.sendall(text.encode("utf-8"))
  return _recv_response(sock)


def _trim_response(text):
  return text.strip(" \t\r\n")


def _json_ok(text):
  try:
    obj = json.loads(text)
  except Exception:
    return False

  return bool(obj.get("ok"))


def SendControlCommand(host, port, protocol, password, command, fmt):
  use_ssl = protocol.lower() == "https"
  real_command = command.strip() or "help"
  real_format = (fmt or "text").lower()

  if real_format == "json" and not real_command.startswith("format "):
    real_command = "format json " + real_command

  raw_sock = socket.create_connection((host, int(port)), timeout=5.0)

  try:
    if use_ssl:
      context = ssl.create_default_context()
      context.check_hostname = False
      context.verify_mode = ssl.CERT_NONE
      sock = context.wrap_socket(raw_sock, server_hostname=host)
    else:
      sock = raw_sock

    try:
      if password:
        auth_command = "auth " + password
        auth_response = _send_request(sock, auth_command)

        if not _trim_response(auth_response).startswith("ok"):
          return {
            "ok": False,
            "error": "auth failed",
            "text": auth_response,
            "json": None
          }

      response = _send_request(sock, real_command)

      parsed = None
      ok = True
      error = None

      if real_format == "json":
        try:
          parsed = json.loads(response)
          ok = bool(parsed.get("ok"))
          error = parsed.get("error")
        except Exception as e:
          ok = False
          error = f"invalid JSON response: {e}"
      else:
        trimmed = _trim_response(response)
        if trimmed.startswith("error:"):
          ok = False
          error = trimmed.splitlines()[0]

      return {
        "ok": ok,
        "error": error,
        "text": response,
        "json": parsed
      }

    finally:
      try:
        if use_ssl:
          sock.unwrap()
      except Exception:
        pass

      sock.close()

  finally:
    try:
      raw_sock.close()
    except Exception:
      pass
