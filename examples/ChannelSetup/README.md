# ChannelSetup example

## ChannelSetup

This example shows how channel visibility is configured.

## What it demonstrates

- Logging to a channel before it is configured
- Creating a channel explicitly
- Making it visible with `AddLink(::CH)`
- The simplest pattern for custom channels

## Notes

- Creating a channel alone is not enough to see output.
- Linking to `::CH` is the easiest way to reuse the default backend configuration.
