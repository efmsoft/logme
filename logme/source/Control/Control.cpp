#include <algorithm>

#ifdef _WIN32
#include <io.h> 
#include <winsock2.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>

#define SOCKADDR_IN_ADDR(pa) (pa)->sin_addr.S_un.S_addr
#define read _read
#define write _write
#else
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#define SD_SEND SHUT_WR
#define SD_RECEIVE SHUT_RD

#define SOCKADDR_IN_ADDR(pa) (pa)->sin_addr.s_addr
#endif

#ifndef _WIN32
#define ioctlsocket ioctl
#define closesocket close
#endif

#include <Logme/Logme.h>
#include <Logme/Utils.h>

#include "CommandDescriptor.h"

using namespace Logme;

void Logger::StopControlServer()
{
  ThreadPtr thread;

  if (true)
  {
    std::lock_guard<std::mutex> guard(DataLock);

    if (ControlSocket != -1)
    {
      shutdown(ControlSocket, SD_RECEIVE);

      closesocket(ControlSocket);
      ControlSocket = -1;
    }

    std::swap(thread, ListenerThread);
  }

  if (thread)
  {
    if (thread->joinable())
      thread->join();

    thread.reset();
  }
}

bool Logger::StartControlServer(const ControlConfig& c)
{
  StopControlServer();

  if (!c.Enable)
    return true;

  std::lock_guard<std::mutex> guard(DataLock);

  ControlSocket = (int)socket(AF_INET, SOCK_STREAM, 0);
  if (ControlSocket == -1)
  {
    LogmeE(CHINT, "Unable to create control socket: %s", OSERR2);
    return false;
  }

  int reuse = true;
  int e = setsockopt(
    ControlSocket
    , SOL_SOCKET
    , SO_REUSEADDR
    , (const char*)&reuse
    , sizeof(reuse)
  );
  if (e < 0)
  {
    LogmeE(CHINT, "setsockopt(SO_REUSEADDR) failed: %s", OSERR2);
    return false;
  }

  struct sockaddr_in servaddr{};
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(c.Interface);
  servaddr.sin_port = htons(c.Port);

  if (bind(ControlSocket, (sockaddr*)&servaddr, sizeof(servaddr)) != 0)
  {
    LogmeE(CHINT, "setsockopt(SO_REUSEADDR) failed: %s", OSERR2);

    closesocket(ControlSocket);
    ControlSocket = -1;

    return false;
  }

  if (listen(ControlSocket, SOMAXCONN) < 0)
  {
    LogmeE(CHINT, "listen() failed: %s", OSERR2);

    closesocket(ControlSocket);
    ControlSocket = -1;

    return false;
  }

  ListenerThread = std::make_shared<std::thread>(&Logger::ControlListener, this);
  return true;
}

void Logger::ControlListener()
{
  RenameThread(-1, "LogCtrlListener");

  for (;;)
  {
    // Listen for new connection
    int accept_sock = (int)accept(ControlSocket, nullptr, nullptr);
    if (accept_sock == -1)
      break;

    // Start ControlHandler to handle new connection
    if (true)
    {
      ControlThread ct{};
      ct.Stopped = false;

      std::lock_guard<std::mutex> guard(DataLock);
      ControlThreads[accept_sock] = ct;

      ControlThreads[accept_sock].Thread = std::make_shared<std::thread>(
        &Logger::ControlHandler
        , this
        , accept_sock
      );
    }

    // Remove items for closed connections
    std::lock_guard<std::mutex> guard(DataLock);
    for (auto it = ControlThreads.begin(); it != ControlThreads.end();)
    {
      if (!it->second.Stopped)
      {
        it++;
        continue;
      }

      closesocket(it->first);
      
      if (it->second.Thread->joinable())
        it->second.Thread->join();

      it->second.Thread.reset();
      it = ControlThreads.erase(it);
    }
  }

  // Shutdown action connections
  if (true)
  {
    std::lock_guard<std::mutex> guard(DataLock);
    for (auto& t : ControlThreads)
    {
      shutdown(t.first, SD_RECEIVE);
      closesocket(t.first);
    }
  }

  // Join connection threads
  std::lock_guard<std::mutex> guard(DataLock);
  for (auto it = ControlThreads.begin(); it != ControlThreads.end();)
  {
    if (it->second.Thread->joinable())
      it->second.Thread->join();

    it->second.Thread.reset();
    it = ControlThreads.erase(it);
  }
}

void Logger::ControlHandler(int socket)
{
  RenameThread(-1, "LogCtrlHandler");
    
  for (;;)
  {
    char buff[4096]{};
    int n = (int)recv(socket, buff, sizeof(buff), 0);
    if (n == -1)
      break;

    std::string command(buff, n);
    std::string response = Control(command);

    if (!response.empty())
    {
      n = (int)send(socket, response.c_str(), (int)response.size(), 0);
      if (n == -1)
        break;
    }
  }

  std::lock_guard<std::mutex> guard(DataLock);
  auto it = ControlThreads.find(socket);
  if (it != ControlThreads.end())
    it->second.Stopped = true;
}

void Logger::SetControlExtension(Logme::TControlHandler handler)
{
  std::lock_guard<std::mutex> guard(DataLock);
  ControlExtension = handler;
}

std::string Logger::Control(const std::string& command)
{
  std::string k(command);
  std::transform(k.begin(), k.end(), k.begin(), ::tolower);

  StringArray items;
  size_t n = WordSplit(k, items);

  std::string response;
  if (ControlExtension)
  {
    if (ControlExtension(command, response))
    {
      if (!n || items[0] != "help")
        return response;
    }
  }

  if (n)
  {
    const std::string& c = items[0];
    for (CommandDescriptor* d = CommandDescriptor::Head; d; d = d->Next)
    {
      if (d->Command != c)
        continue;

      if (d->Handler(items, response))
        return response;
    }
  }

  return std::string("unsupported command: ") + (n ? items[0] : "<empty>");
}