# logme examples

Examples are built only when `LOGME_BUILD_EXAMPLES=ON`.

## Build

```bash
cmake -S . -B out/build -DLOGME_BUILD_EXAMPLES=ON
cmake --build out/build
```

Example binaries are placed under the build tree (for Ninja, typically under `out/build/Examples/*`).

## Notes

### Channels, backends, and links

- You can pass any `Logme::ID` to `LogmeI/LogmeW/LogmeE/LogmeD`.
- **To actually see output**, the target channel must have at least one backend, or it must be linked to another channel that has backends.
- The default channel `::CH` is created automatically and has console output enabled by default.

The simplest way to make a custom channel visible is:

```cpp
auto ch = Logme::Instance->CreateChannel(id);
ch->AddLink(::CH);
```

### `ChannelPtr` vs `Logme::ID`

You can pass either a `Logme::ID` or a `ChannelPtr` to the logging macros.
Using `ChannelPtr` avoids name lookup and can reduce contention in multi-threaded code (especially when different threads write to different channels).

### Thread context helpers

`LogmeThreadChannel` (and the analogous helper for overrides) can set the default channel/override for the current thread.
This allows libraries to log without explicitly receiving a channel argument, while still routing their output to the right place.

### One-time messages

`OneTimeOverrideGenerator` + `LOGME_ONCE4THIS` allows printing a message only once per object, even if the method is called many times.
