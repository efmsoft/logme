# LogmeWebManual

`LogmeWebManual` is a manual smoke-test host for `tools/logmeweb`.
It is built as part of the test tree but is not registered as a CTest test because it requires a browser and stays alive until ENTER is pressed.

Typical runs:

```bat
LogmeWebManual.exe --http 7791
LogmeWebManual.exe --http 7791 --password test
LogmeWebManual.exe --https 7792
LogmeWebManual.exe --ssl --password test
LogmeWebManual.exe --http 7791 --obfuscation on
```

After startup, run `tools/logmeweb/logmeweb.py`, connect to the printed URL, and verify:

1. Overview counters change while the host is running.
2. Channels can be enabled and disabled.
3. FileBackend, ConsoleBackend, and BufferBackend details are shown.
4. Additional FileBackend instances can be added from the UI.
5. Trace points can be enabled and disabled, changing which trace-point records are printed.
6. Manual command supports `help`, `overview`, `channels`, `trace list`, and `trace stats`.
7. Password-protected mode works when `--password` is used.
8. HTTPS mode works when `--https` or `--ssl` is used. If `--cert` and `--key` are omitted, the host generates local self-signed certificate/key files near the executable. These generated files are only for local manual testing and must not be committed.
9. Obfuscation mode works when `--obfuscation on` is used.
