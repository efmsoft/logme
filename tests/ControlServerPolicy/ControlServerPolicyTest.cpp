#include <gtest/gtest.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <Logme/Logme.h>

#include <cstring>
#include <string>

using namespace Logme;

namespace
{
  ID CHT{ "control_server_policy" };
  ChannelPtr Ch;

#ifdef _WIN32
  bool InitializeSockets()
  {
    static bool initialized = []() -> bool
    {
      WSADATA wsa{};
      return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
    }();

    return initialized;
  }

  void CloseSocket(SOCKET socket)
  {
    closesocket(socket);
  }
#else
  bool InitializeSockets()
  {
    return true;
  }

  void CloseSocket(int socket)
  {
    close(socket);
  }
#endif

  int FindFreePort()
  {
    if (!InitializeSockets())
      return 0;

#ifdef _WIN32
    SOCKET socketHandle = socket(AF_INET, SOCK_STREAM, 0);
    if (socketHandle == INVALID_SOCKET)
      return 0;
#else
    int socketHandle = socket(AF_INET, SOCK_STREAM, 0);
    if (socketHandle < 0)
      return 0;
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    int result = bind(
      socketHandle
      , reinterpret_cast<sockaddr*>(&addr)
      , sizeof(addr)
    );

    if (result != 0)
    {
      CloseSocket(socketHandle);
      return 0;
    }

#ifdef _WIN32
    int len = sizeof(addr);
#else
    socklen_t len = sizeof(addr);
#endif

    result = getsockname(
      socketHandle
      , reinterpret_cast<sockaddr*>(&addr)
      , &len
    );

    if (result != 0)
    {
      CloseSocket(socketHandle);
      return 0;
    }

    int port = ntohs(addr.sin_port);
    CloseSocket(socketHandle);
    return port;
  }

  std::string SendControlCommand(
    int port
    , const std::string& command
  )
  {
    if (!InitializeSockets())
      return std::string();

#ifdef _WIN32
    SOCKET socketHandle = socket(AF_INET, SOCK_STREAM, 0);
    if (socketHandle == INVALID_SOCKET)
      return std::string();
#else
    int socketHandle = socket(AF_INET, SOCK_STREAM, 0);
    if (socketHandle < 0)
      return std::string();
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(static_cast<unsigned short>(port));

    int result = connect(
      socketHandle
      , reinterpret_cast<sockaddr*>(&addr)
      , sizeof(addr)
    );

    if (result != 0)
    {
      CloseSocket(socketHandle);
      return std::string();
    }

#ifdef _WIN32
    result = send(
      socketHandle
      , command.c_str()
      , static_cast<int>(command.size())
      , 0
    );
#else
    result = send(
      socketHandle
      , command.c_str()
      , command.size()
      , 0
    );
#endif

    if (result <= 0)
    {
      CloseSocket(socketHandle);
      return std::string();
    }

    char buffer[4096]{};
#ifdef _WIN32
    result = recv(socketHandle, buffer, static_cast<int>(sizeof(buffer)), 0);
#else
    result = recv(socketHandle, buffer, sizeof(buffer), 0);
#endif

    CloseSocket(socketHandle);

    if (result <= 0)
      return std::string();

    return std::string(buffer, static_cast<size_t>(result));
  }

  bool Contains(
    const std::string& text
    , const std::string& value
  )
  {
    return text.find(value) != std::string::npos;
  }

  void ResetLoggerState()
  {
    Logme::Instance->StopControlServer();
    Ch->SetFilterLevel(Logme::LEVEL_INFO);

    (void)Logme::Instance->Control(
      "backend --channel control_server_policy --delete ring"
    );
  }
}

TEST(ControlServerPolicy, SafePolicyRejectsBackendChanges)
{
  ResetLoggerState();

  int port = FindFreePort();
  ASSERT_NE(port, 0);

  Logme::ControlConfig cfg{};
  cfg.Enable = true;
  cfg.Port = port;
  cfg.Interface = 0;
  cfg.DiscoveryEnable = false;

  ASSERT_TRUE(Logme::Instance->StartControlServer(cfg, ControlPolicy::Safe()));

  std::string backendResponse = SendControlCommand(
    port
    , "backend --channel control_server_policy --add ring --max-items 3"
  );

  EXPECT_TRUE(Contains(backendResponse, "rejected by control policy"));

  std::string levelResponse = SendControlCommand(
    port
    , "level --channel control_server_policy debug"
  );

  EXPECT_TRUE(Contains(levelResponse, "ok"));
  EXPECT_EQ(Ch->GetFilterLevel(), Logme::LEVEL_DEBUG);

  Logme::Instance->StopControlServer();
}

TEST(ControlServerPolicy, PolicyCanBeChangedForRunningServer)
{
  ResetLoggerState();

  int port = FindFreePort();
  ASSERT_NE(port, 0);

  Logme::ControlConfig cfg{};
  cfg.Enable = true;
  cfg.Port = port;
  cfg.Interface = 0;
  cfg.DiscoveryEnable = false;

  ASSERT_TRUE(Logme::Instance->StartControlServer(cfg, ControlPolicy::Safe()));

  std::string safeResponse = SendControlCommand(
    port
    , "backend --channel control_server_policy --add ring --max-items 3"
  );

  EXPECT_TRUE(Contains(safeResponse, "rejected by control policy"));

  Logme::Instance->SetControlServerPolicy(ControlPolicy::Diagnostic());

  std::string diagnosticResponse = SendControlCommand(
    port
    , "backend --channel control_server_policy --add ring --max-items 3"
  );

  EXPECT_TRUE(Contains(diagnosticResponse, "ok"));

  std::string cleanupResponse = Logme::Instance->Control(
    "backend --channel control_server_policy --delete ring"
  );

  EXPECT_EQ(cleanupResponse, "ok");

  Logme::Instance->StopControlServer();
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  Ch = Logme::Instance->CreateChannel(CHT);
  Ch->SetFilterLevel(Logme::LEVEL_INFO);

  int result = RUN_ALL_TESTS();

  Logme::Instance->StopControlServer();
  return result;
}
