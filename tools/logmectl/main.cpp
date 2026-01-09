#include <Logme/Logme.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "Ws2_32.lib")

  static int CloseSocket(SOCKET s)
  {
    return closesocket(s);
  }
#else
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <sys/types.h>
  #include <unistd.h>
  #include <errno.h>

  typedef int SOCKET;
  #define INVALID_SOCKET (-1)
  #define SOCKET_ERROR (-1)

  static int CloseSocket(SOCKET s)
  {
    return close(s);
  }
#endif

static bool SetRecvTimeout(
  SOCKET s
  , int timeoutMs
  )
{
#ifdef _WIN32
  DWORD tv = (DWORD)timeoutMs;
  return setsockopt(
    s
    , SOL_SOCKET
    , SO_RCVTIMEO
    , (const char*)&tv
    , (int)sizeof(tv)
  ) == 0;
#else
  timeval tv = {0};
  tv.tv_sec = timeoutMs / 1000;
  tv.tv_usec = (timeoutMs % 1000) * 1000;
  return setsockopt(
    s
    , SOL_SOCKET
    , SO_RCVTIMEO
    , (const char*)&tv
    , (socklen_t)sizeof(tv)
  ) == 0;
#endif
}


static void PrintUsage()
{
  std::cout
    << "Logme control\n\n"
    << "Usage:\n"
    << "  logmectl -p <port> [-i <ip>] <command...>\n\n"
    << "Run with \"help\" to see available commands.\n";
}

static bool ParseArgs(
  int argc
  , char** argv
  , std::string& ip
  , int& port
  , std::string& command
)
{
  ip = "127.0.0.1";
  port = 0;

  std::vector<std::string> cmd;

  for (int i = 1; i < argc; ++i)
  {
    const char* a = argv[i];

    if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0)
    {
      return false;
    }

    if ((strcmp(a, "-p") == 0 || strcmp(a, "--port") == 0) && i + 1 < argc)
    {
      port = atoi(argv[++i]);
      continue;
    }

    if ((strcmp(a, "-i") == 0 || strcmp(a, "--ip") == 0) && i + 1 < argc)
    {
      ip = argv[++i];
      continue;
    }

    cmd.emplace_back(argv[i]);
  }

  if (port <= 0)
    return false;

  if (cmd.empty())
    command = "help";
  else
  {
    command.clear();
    for (size_t i = 0; i < cmd.size(); ++i)
    {
      if (i)
        command.push_back(' ');

      command += cmd[i];
    }
  }

  return true;
}

static bool ResolveAndConnect(
  const std::string& ip
  , int port
  , SOCKET& out
  , std::string& error
)
{
  out = INVALID_SOCKET;

#ifdef _WIN32
  WSADATA wsa{};
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
  {
    error = "WSAStartup failed";
    return false;
  }
#endif

  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  addrinfo* res = nullptr;
  const std::string portStr = std::to_string(port);

  int r = getaddrinfo(ip.c_str(), portStr.c_str(), &hints, &res);
  if (r != 0 || !res)
  {
#ifdef _WIN32
    error = std::string("getaddrinfo failed: ") + gai_strerrorA(r);
#else
    error = std::string("getaddrinfo failed: ") + gai_strerror(r);
#endif
    return false;
  }

  for (addrinfo* p = res; p; p = p->ai_next)
  {
    SOCKET s = (SOCKET)socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (s == INVALID_SOCKET)
      continue;

    if (connect(s, p->ai_addr, (int)p->ai_addrlen) != SOCKET_ERROR)
    {
      out = s;
      break;
    }

    CloseSocket(s);
  }

  freeaddrinfo(res);

  if (out == INVALID_SOCKET)
  {
    error = "connect failed";
    return false;
  }

  // Avoid hanging forever if the server keeps the socket open.
#ifdef _WIN32
  DWORD ms = 1000;
  setsockopt(out, SOL_SOCKET, SO_RCVTIMEO, (const char*)&ms, sizeof(ms));
#else
  timeval tv{};
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  setsockopt(out, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
#endif

  return true;
}

int main(int argc, char** argv)
{
  std::string ip;
  int port = 0;
  std::string cmd;

  if (!ParseArgs(argc, argv, ip, port, cmd))
  {
    PrintUsage();
    return 2;
  }

  SOCKET s = INVALID_SOCKET;
  std::string err;

  if (!ResolveAndConnect(ip, port, s, err))
  {
    std::cerr << "ERROR: " << err << "\n";
    return 1;
  }

  int sent = send(s, cmd.c_str(), (int)cmd.size(), 0);
  if (sent == SOCKET_ERROR)
  {
    std::cerr << "ERROR: send failed\n";
    CloseSocket(s);
    return 1;
  }

  std::string response;

  // Wait for the first response packet with a hard timeout, then drain
  // any remaining packets with a short inter-packet timeout.
  (void)SetRecvTimeout(s, 5000);
  bool gotFirst = false;

  for (;;)
  {
    char buf[4096];
    int n = (int)recv(s, buf, (int)sizeof(buf), 0);

    if (n > 0)
    {
      response.append(buf, n);

      if (!gotFirst)
      {
        gotFirst = true;
        (void)SetRecvTimeout(s, 100);
      }

      continue;
    }

    if (n == 0)
      break;

#ifdef _WIN32
    int e = WSAGetLastError();
    if (e == WSAETIMEDOUT || e == WSAEWOULDBLOCK)
      break;
#else
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      break;
#endif

    break;
  }

  if (!response.empty())
    std::cout << response;

  CloseSocket(s);

#ifdef _WIN32
  WSACleanup();
#endif

  return 0;
}
