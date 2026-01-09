# Dynamic control demo

This example shows how to enable **runtime control** of Logme and change settings from another terminal.

## What it does

- Starts the Logme **control server** (TCP, no SSL).
- Spawns two worker threads (`t1`, `t2`).
- Each thread logs:
  - to the default channel,
  - to its own channel (`t1`/`t2`),
  - and then to the same channel with two different **subsystem IDs**.

The example prints the control server address and a list of useful commands on startup.

## Build

Enable the example and the tool:

```bash
cmake -S . -B build -G Ninja \
  -DLOGME_BUILD_EXAMPLES=ON \
  -DLOGME_BUILD_TOOLS=ON
cmake --build build -j
```

## Run

Terminal 1:

```bash
./DynamicControl
```

Terminal 2 (replace the port if you changed it):

```bash
./logmectl -p 7791 list
./logmectl -p 7791 channel disable t1
./logmectl -p 7791 channel enable t1
./logmectl -p 7791 channel level t2 error

./logmectl -p 7791 subsystem list
./logmectl -p 7791 subsystem mode blacklist
./logmectl -p 7791 subsystem add wrk1
```

## Notes

- `subsystem mode blacklist` blocks subsystems present in the list.
- `subsystem mode whitelist` allows only subsystems present in the list.
