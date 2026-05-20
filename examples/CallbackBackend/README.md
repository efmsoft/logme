# CallbackBackend example

This example shows how to route log records to a user callback.

`CallbackBackend` is useful when an application wants to collect records in memory, forward them to an existing telemetry pipeline, or integrate logme with a custom diagnostics system.

The callback receives the original `Logme::Context`, the owning channel, and the user data pointer supplied when the backend is created.
The example formats the record with the owning channel flags and prints it from the callback.
