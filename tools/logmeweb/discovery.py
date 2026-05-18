import json
import os
import platform
import socket

try:
  import psutil
except Exception:
  psutil = None

try:
  import win32file
  import pywintypes
except Exception:
  win32file = None
  pywintypes = None


WINDOWS_PIPE_PREFIX = r"\\.\pipe\Global\logme-discovery-"
UNIX_SOCKET_PREFIX = "logme-discovery-"


def _iter_pids():
  if psutil:
    for proc in psutil.process_iter(["pid"]):
      try:
        yield int(proc.info["pid"]), ""
      except Exception:
        continue
    return

  proc_dir = "/proc"
  if os.path.isdir(proc_dir):
    for name in os.listdir(proc_dir):
      if name.isdigit():
        yield int(name), ""


def _query_windows_pipe(pid):
  if not win32file:
    return None

  pipe_name = f"{WINDOWS_PIPE_PREFIX}{pid}"

  try:
    handle = win32file.CreateFile(
      pipe_name
      , win32file.GENERIC_READ | win32file.GENERIC_WRITE
      , 0
      , None
      , win32file.OPEN_EXISTING
      , 0
      , None
    )

    try:
      win32file.WriteFile(handle, b"INFO\n")
      result, data = win32file.ReadFile(handle, 64 * 1024)
      return json.loads(data.decode("utf-8", errors="replace"))
    finally:
      win32file.CloseHandle(handle)

  except pywintypes.error:
    return None
  except Exception:
    return None


def _query_unix_socket_address(address):
  s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
  s.settimeout(0.2)

  try:
    s.connect(address)
    s.sendall(b"INFO\n")
    data = s.recv(64 * 1024)
    if not data:
      return None

    return json.loads(data.decode("utf-8", errors="replace"))
  except Exception:
    return None
  finally:
    s.close()


def _query_unix_socket(pid):
  if platform.system() == "Linux":
    info = _query_unix_socket_address("\0" + UNIX_SOCKET_PREFIX + str(pid))
    if info:
      return info

  candidates = [
    f"/tmp/{UNIX_SOCKET_PREFIX}{pid}.sock",
    f"/tmp/{UNIX_SOCKET_PREFIX}{pid}"
  ]

  for path in candidates:
    if not os.path.exists(path):
      continue

    info = _query_unix_socket_address(path)
    if info:
      return info

  return None


def DiscoverProcesses():
  results = []
  seen = set()
  is_windows = platform.system() == "Windows"

  for pid, fallback_name in _iter_pids():
    info = _query_windows_pipe(pid) if is_windows else _query_unix_socket(pid)
    if not info:
      continue

    if "pid" not in info:
      info["pid"] = pid

    if "process" not in info or not info["process"]:
      info["process"] = fallback_name or str(pid)

    key = (info.get("pid"), info.get("control", {}).get("port"))
    if key in seen:
      continue

    seen.add(key)
    results.append(info)

  results.sort(key=lambda x: (str(x.get("process", "")).lower(), int(x.get("pid", 0))))
  return results
