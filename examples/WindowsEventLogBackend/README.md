# WindowsEventLogBackend example

This Windows-only example writes logme records to Windows Event Log.

The example creates a separate channel, attaches `WindowsEventLogBackend`, sets the event source, event id, category, and enables asynchronous delivery.
It is intentionally excluded from non-Windows builds.

The example target links `ws2_32` together with logme, matching other Windows examples in this tree.
