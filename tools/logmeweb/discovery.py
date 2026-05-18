import ctypes
import json
import os
import platform
import socket
import sys

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


def _debug(message):
  if os.environ.get("LOGMEWEB_DISCOVERY_DEBUG"):
    print(message, file=sys.stderr)


def _iter_windows_pids():
  kernel32 = ctypes.windll.kernel32
  snapshot = kernel32.CreateToolhelp32Snapshot(0x00000002, 0)
  if snapshot == ctypes.c_void_p(-1).value:
    return

  class PROCESSENTRY32(ctypes.Structure):
    _fields_ = [
      ("dwSize", ctypes.c_uint32),
      ("cntUsage", ctypes.c_uint32),
      ("th32ProcessID", ctypes.c_uint32),
      ("th32DefaultHeapID", ctypes.c_void_p),
      ("th32ModuleID", ctypes.c_uint32),
      ("cntThreads", ctypes.c_uint32),
      ("th32ParentProcessID", ctypes.c_uint32),
      ("pcPriClassBase", ctypes.c_long),
      ("dwFlags", ctypes.c_uint32),
      ("szExeFile", ctypes.c_char * 260),
    ]

  entry = PROCESSENTRY32()
  entry.dwSize = ctypes.sizeof(PROCESSENTRY32)

  try:
    ok = kernel32.Process32First(snapshot, ctypes.byref(entry))
    while ok:
      yield int(entry.th32ProcessID), ""
      ok = kernel32.Process32Next(snapshot, ctypes.byref(entry))
  finally:
    kernel32.CloseHandle(snapshot)


def _iter_pids():
  if platform.system() == "Windows":
    yield from _iter_windows_pids()
    return

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


def _query_windows_pipe_with_pywintypes(pipe_name):
  if not win32file:
    return None

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

  except Exception as e:
    _debug(f"pipe={pipe_name} pywin32_error={e}")
    return None


def _query_windows_pipe_with_ctypes(pipe_name):
  kernel32 = ctypes.windll.kernel32
  GENERIC_READ = 0x80000000
  GENERIC_WRITE = 0x40000000
  OPEN_EXISTING = 3
  INVALID_HANDLE_VALUE = ctypes.c_void_p(-1).value

  handle = kernel32.CreateFileW(
    ctypes.c_wchar_p(pipe_name)
    , GENERIC_READ | GENERIC_WRITE
    , 0
    , None
    , OPEN_EXISTING
    , 0
    , None
  )

  if handle == INVALID_HANDLE_VALUE:
    error = kernel32.GetLastError()
    _debug(f"pipe={pipe_name} open_error={error}")
    return None

  try:
    request = b"INFO\n"
    written = ctypes.c_uint32(0)
    ok = kernel32.WriteFile(
      handle
      , request
      , len(request)
      , ctypes.byref(written)
      , None
    )
    if not ok:
      error = kernel32.GetLastError()
      _debug(f"pipe={pipe_name} write_error={error}")
      return None

    buffer = ctypes.create_string_buffer(64 * 1024)
    read = ctypes.c_uint32(0)
    ok = kernel32.ReadFile(
      handle
      , buffer
      , ctypes.sizeof(buffer)
      , ctypes.byref(read)
      , None
    )
    if not ok:
      error = kernel32.GetLastError()
      _debug(f"pipe={pipe_name} read_error={error}")
      return None

    data = buffer.raw[:read.value]
    return json.loads(data.decode("utf-8", errors="replace"))
  finally:
    kernel32.CloseHandle(handle)


def _query_windows_pipe(pid):
  pipe_name = f"{WINDOWS_PIPE_PREFIX}{pid}"

  info = _query_windows_pipe_with_pywintypes(pipe_name)
  if info:
    return info

  return _query_windows_pipe_with_ctypes(pipe_name)


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
