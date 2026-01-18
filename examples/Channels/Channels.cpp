#include <Logme/Logme.h>

#include <Logme/Backend/ConsoleBackend.h>

#include <memory>

#if defined(_MSC_VER)
#pragma warning(disable: 4840)
#endif

int main()
{
  // NOTE:
  // You can log to any channel ID, but to actually see output you must:
  //   1) Create the channel (if it does not exist yet)
  //   2) Add one or more backends, OR link it to another channel that has backends
  //
  // The default channel (::CH) is created automatically. It prints to console and, on Windows,
  // also to the debugger output (OutputDebugStringA).

  Logme::ID net{"net"};
  Logme::ID fs{"fs"};
  Logme::ID ui{"ui"};

  // Option A: create a channel and attach a backend directly.
  auto netCh = Logme::Instance->CreateChannel(net);
  netCh->AddBackend(std::make_shared<Logme::ConsoleBackend>(netCh));

  // Option B: link a channel to the default channel.
  // A linked channel logs to its own backends first, then forwards to the linked channel.
  auto fsCh = Logme::Instance->CreateChannel(fs);
  fsCh->AddLink(::CH);

  auto uiCh = Logme::Instance->CreateChannel(ui);
  uiCh->AddLink(::CH);

  // You can pass Channel ID...
  LogmeI(net) << "Connected";
  LogmeW(net, "RTT=%d ms", 12);

  // ...or ChannelPtr (slightly faster, avoids name lookup).
  LogmeI(netCh) << "Connected (ChannelPtr)";
  LogmeW(netCh, "RTT=%d ms (ChannelPtr)", 12);

  LogmeI(fs) << "Open file";
  LogmeE(fs, "Open failed: %s", "access denied");

  LogmeI(ui) << "Button clicked";

  return 0;
}
