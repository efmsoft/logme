# CApi example

This example shows the minimal C interface exported by logme.

The C API is intentionally smaller than the C++ API. It supports printf-style logging macros, optional channel and subsystem arguments through dedicated macro variants, basic configuration loading, channel level control, channel enable/disable control, and shutdown.

The main C macros keep the familiar names:

```c
LogmeD("debug value: %d", value);
LogmeI("info message");
LogmeW("warning message");
LogmeE("error message");
LogmeC("critical message");
```

Named channels and subsystem ids are available through explicit C macro variants:

```c
LogmeI_Ch("network", "connected");
LogmeE_Sid("HTTP", "request failed: %d", status);
LogmeW_ChSid("network", "HTTP", "retrying");
```

C does not support the C++ stream-style API or the overloaded macro argument dispatch used by the C++ interface.
