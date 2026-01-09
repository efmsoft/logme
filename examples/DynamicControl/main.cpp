#ifdef _WIN32
#include <winsock2.h>
#endif

#include "Worker.h"

#include <iostream>
#include <string>

#include <Logme/Logger.h>
#include <Logme/Logme.h>

static void PrintInstructions(int port)
{
  std::cout
    << "Dynamic control demo\n"
    << "\n"
    << "This example starts Logme control server and prints from two worker threads.\n"
    << "\n"
    << "There are two channels: ch1 and ch2. Each channel has two subsystems:\n"
    << "  ch1: t1s1, t1s2\n"
    << "  ch2: t2s1, t2s2\n"
    << "\n"
    << "Initially the channels are inactive because they have no backends attached.\n"
    << "To activate a channel, add a backend, for example:\n"
    << "  logmectl -p " << port << " backend --channel ch1 --add console\n"
    << "\n"
    << "After adding a backend, you will see messages sent to the channel and to subsystems\n"
    << "because the 'block reported subsystems' mode is enabled.\n"
    << "\n"
    << "You can switch the mode off:\n"
    << "  logmectl -p " << port << " subsystem --unblock-reported\n"
    << "\n"
    << "Then only messages with the channel specified will be visible.\n"
    << "To see messages with both channel and subsystem again, report a subsystem:\n"
    << "  logmectl -p " << port << " subsystem --report t1s1\n"
    << "\n"
    << "Control server: 127.0.0.1:" << port << "\n"
    << "\n"
    << "Press ENTER to stop.\n"
    << std::endl;
}


int main()
{
#ifdef _WIN32
  WSADATA wsa = {0};
  auto rc = WSAStartup(MAKEWORD(2, 2), &wsa);
  if (rc != 0)
  {
    std::cerr << "WSAStartup failed: " << rc << std::endl;
    return 1;
  }
#endif

  const int port = 7791;
  Logme::ControlConfig cfg{};
  cfg.Enable = true;
  cfg.Port = port;
  cfg.Interface = 0;

  if (!Logme::Instance->StartControlServer(cfg))
  {
    std::cerr << "Failed to start control server" << std::endl;

#ifdef _WIN32
  WSACleanup();
#endif
    return 2;
  }

  PrintInstructions(port);

  Worker w1("t1", "ch1", "t1s1", "t1s2");
  Worker w2("t2", "ch2", "t2s1", "t2s2");

  w1.Start();
  w2.Start();

  std::string line;
  std::getline(std::cin, line);

  w1.Stop();
  w2.Stop();

#ifdef _WIN32
  WSACleanup();
#endif

  return 0;
}
