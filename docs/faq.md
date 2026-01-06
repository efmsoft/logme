# FAQ

## I created a channel but see no output. Why?

Channels do not have backends by default.
You must explicitly attach at least one backend:

```cpp
auto ch = Logme::Instance->CreateChannel("my");
ch->AddBackend(std::make_shared<ConsoleBackend>(ch));
```

## Why is this behavior intentional?

Implicit global output often causes confusion in large systems.
logme requires explicit routing to keep logging predictable.

## Why is std::format sometimes disabled?

`std::format` depends on the standard library implementation.

Examples:
- GCC 11 (Ubuntu 22.04): `<format>` is not available
- GCC 13+: `<format>` is available

When `LOGME_STD_FORMAT=AUTO` is used, logme enables format support
only if the toolchain provides `<format>`.

## How can I force format behavior?

- `-DLOGME_STD_FORMAT=ON`  — require std::format
- `-DLOGME_STD_FORMAT=OFF` — disable std::format unconditionally
