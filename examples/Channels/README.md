# Channels example

## Channels

This example shows how to work with multiple channels.

## What it demonstrates

- Creating named channels
- Making a channel visible with a backend
- Making a channel visible by linking it to `::CH`
- Logging with either `Logme::ID` or `ChannelPtr`

## Notes

- A custom channel does not produce visible output until it has backends or links.
- Passing `ChannelPtr` avoids channel name lookup.
