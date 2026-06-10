# EnvironmentControl

## EnvironmentControl

This example shows explicit startup control from process environment variables.

Environment control is disabled unless the application calls `ApplyEnvironmentControl()`.
For generated documentation and simple local runs, the example is self-contained: if `LOGME_CONTROL` is not already set, the example sets it internally before calling `ApplyEnvironmentControl()`.

The demo channel `env_example` is initially disabled. Messages sent to it before `ApplyEnvironmentControl()` are not printed. The environment command enables the channel and changes its level to `debug`, so the messages printed after `ApplyEnvironmentControl()` become visible.

Default demo command:

```sh
LOGME_CONTROL="channel --enable env_example; level --channel env_example debug"
```

You can override it from the shell:

```sh
LOGME_CONTROL="channel --enable env_example; level --channel env_example info" ./EnvironmentControl
```

The string may contain multiple control commands separated by `;`. Each command is executed through the normal control API and is checked by the policy passed to `ApplyEnvironmentControl()`.
