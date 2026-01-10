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
  #include <windows.h>
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
  #include <dlfcn.h>

  typedef int SOCKET;
  #define INVALID_SOCKET (-1)
  #define SOCKET_ERROR (-1)

  static int CloseSocket(SOCKET s)
  {
    return close(s);
  }
#endif

namespace
{
#if defined(_WIN32)
  typedef HMODULE SslLibHandle;
#else
  typedef void* SslLibHandle;
#endif

    std::string BuildTriedLibraryNames(const char* const* names)
  {
    std::string out;
    for (const char* const* p = names; *p; ++p)
    {
      if (!out.empty())
        out += ", ";
      out += *p;
    }
    return out;
  }

SslLibHandle SslLoadLibrary(const char* const* names)
  {
    for (const char* const* p = names; *p; ++p)
    {
#if defined(_WIN32)
      HMODULE h = LoadLibraryA(*p);
      if (h)
        return h;
#else
      void* h = dlopen(*p, RTLD_NOW);
      if (h)
        return h;
#endif
    }

    return nullptr;
  }

  void* SslGetSymbol(SslLibHandle handle, const char* name)
  {
    if (!handle)
      return nullptr;

#if defined(_WIN32)
    return reinterpret_cast<void*>(GetProcAddress(handle, name));
#else
    return dlsym(handle, name);
#endif
  }

  void SslCloseLibrary(SslLibHandle handle)
  {
#if defined(_WIN32)
    if (handle)
      FreeLibrary(handle);
#else
    if (handle)
      dlclose(handle);
#endif
  }

  struct ClientSslContext
  {
    SslLibHandle LibSsl;
    SslLibHandle LibCrypto;

    int (*SSL_library_init_ptr)();
    void (*SSL_load_error_strings_ptr)();
    void* (*TLS_client_method_ptr)();

    void* (*SSL_CTX_new_ptr)(void*);
    void (*SSL_CTX_free_ptr)(void*);

    void* (*SSL_new_ptr)(void*);
    void (*SSL_free_ptr)(void*);
    int (*SSL_set_fd_ptr)(void*, int);
    int (*SSL_connect_ptr)(void*);
    int (*SSL_read_ptr)(void*, void*, int);
    int (*SSL_write_ptr)(void*, const void*, int);
    int (*SSL_shutdown_ptr)(void*);

    void* Ctx;

    ClientSslContext()
      : LibSsl(nullptr)
      , LibCrypto(nullptr)
      , SSL_library_init_ptr(nullptr)
      , SSL_load_error_strings_ptr(nullptr)
      , TLS_client_method_ptr(nullptr)
      , SSL_CTX_new_ptr(nullptr)
      , SSL_CTX_free_ptr(nullptr)
      , SSL_new_ptr(nullptr)
      , SSL_free_ptr(nullptr)
      , SSL_set_fd_ptr(nullptr)
      , SSL_connect_ptr(nullptr)
      , SSL_read_ptr(nullptr)
      , SSL_write_ptr(nullptr)
      , SSL_shutdown_ptr(nullptr)
      , Ctx(nullptr)
    {
    }

    ~ClientSslContext()
    {
      Cleanup();
    }

    void Cleanup()
    {
      if (Ctx && SSL_CTX_free_ptr)
        SSL_CTX_free_ptr(Ctx);

      Ctx = nullptr;

      SslCloseLibrary(LibSsl);
      SslCloseLibrary(LibCrypto);

      LibSsl = nullptr;
      LibCrypto = nullptr;
    }

    bool Init(std::string& error)
    {
#if defined(_WIN32)
#if defined(_WIN64)
      static const char* SSL_NAMES[] =
      {
        "libssl-3-x64.dll",
        "libssl-1_1-x64.dll",
        "libssl-1_1.dll",
        "libssl.dll",
        nullptr
      };
      static const char* CRYPTO_NAMES[] =
      {
        "libcrypto-3-x64.dll",
        "libcrypto-1_1-x64.dll",
        "libcrypto-1_1.dll",
        "libcrypto.dll",
        nullptr
      };
#else
      static const char* SSL_NAMES[] =
      {
        "libssl-3.dll",
        "libssl-1_1.dll",
        "libssl.dll",
        nullptr
      };
      static const char* CRYPTO_NAMES[] =
      {
        "libcrypto-3.dll",
        "libcrypto-1_1.dll",
        "libcrypto.dll",
        nullptr
      };
#endif
#else
      static const char* SSL_NAMES[] =
      {
        "libssl.so.3",
        "libssl.so.1.1",
        "libssl.so",
        nullptr
      };
      static const char* CRYPTO_NAMES[] =
      {
        "libcrypto.so.3",
        "libcrypto.so.1.1",
        "libcrypto.so",
        nullptr
      };
#endif

      LibSsl = SslLoadLibrary(SSL_NAMES);
      if (!LibSsl)
      {
        std::string tried = BuildTriedLibraryNames(SSL_NAMES);
#if !defined(_WIN32)
        const char* dlerr = dlerror();
        if (dlerr && *dlerr)
          error = std::string("Unable to load libssl shared library. Tried: ") + tried + ". (" + dlerr + ")";
        else
          error = std::string("Unable to load libssl shared library. Tried: ") + tried;
#else
        error = std::string("Unable to load libssl shared library. Tried: ") + tried;
#endif
        return false;
      }

      LibCrypto = SslLoadLibrary(CRYPTO_NAMES);
      if (!LibCrypto)
      {
        std::string tried = BuildTriedLibraryNames(CRYPTO_NAMES);
#if !defined(_WIN32)
        const char* dlerr = dlerror();
        if (dlerr && *dlerr)
          error = std::string("Unable to load libcrypto shared library. Tried: ") + tried + ". (" + dlerr + ")";
        else
          error = std::string("Unable to load libcrypto shared library. Tried: ") + tried;
#else
        error = std::string("Unable to load libcrypto shared library. Tried: ") + tried;
#endif
        return false;
      }

#define LOAD_REQUIRED(ptr, lib, name)                                     \
      do                                                                  \
      {                                                                   \
        ptr = reinterpret_cast<decltype(ptr)>(SslGetSymbol(lib, name));   \
        if (!ptr)                                                        \
        {                                                                 \
          error = std::string("Missing OpenSSL symbol: ") + name;        \
          return false;                                                   \
        }                                                                 \
      } while (false)

      SSL_library_init_ptr = reinterpret_cast<decltype(SSL_library_init_ptr)>(
        SslGetSymbol(LibSsl, "SSL_library_init"));
      SSL_load_error_strings_ptr = reinterpret_cast<decltype(SSL_load_error_strings_ptr)>(
        SslGetSymbol(LibSsl, "SSL_load_error_strings"));

      LOAD_REQUIRED(TLS_client_method_ptr, LibSsl, "TLS_client_method");
      LOAD_REQUIRED(SSL_CTX_new_ptr, LibSsl, "SSL_CTX_new");
      LOAD_REQUIRED(SSL_CTX_free_ptr, LibSsl, "SSL_CTX_free");
      LOAD_REQUIRED(SSL_new_ptr, LibSsl, "SSL_new");
      LOAD_REQUIRED(SSL_free_ptr, LibSsl, "SSL_free");
      LOAD_REQUIRED(SSL_set_fd_ptr, LibSsl, "SSL_set_fd");
      LOAD_REQUIRED(SSL_connect_ptr, LibSsl, "SSL_connect");
      LOAD_REQUIRED(SSL_read_ptr, LibSsl, "SSL_read");
      LOAD_REQUIRED(SSL_write_ptr, LibSsl, "SSL_write");
      LOAD_REQUIRED(SSL_shutdown_ptr, LibSsl, "SSL_shutdown");

#undef LOAD_REQUIRED

      if (SSL_library_init_ptr)
        SSL_library_init_ptr();

      if (SSL_load_error_strings_ptr)
        SSL_load_error_strings_ptr();

      void* method = TLS_client_method_ptr();
      if (!method)
      {
        error = "TLS_client_method() returned null";
        return false;
      }

      Ctx = SSL_CTX_new_ptr(method);
      if (!Ctx)
      {
        error = "SSL_CTX_new() failed";
        return false;
      }

      return true;
    }

    void* CreateSession(int socket, std::string& error)
    {
      void* ssl = SSL_new_ptr(Ctx);
      if (!ssl)
      {
        error = "SSL_new() failed";
        return nullptr;
      }

      if (SSL_set_fd_ptr(ssl, socket) != 1)
      {
        error = "SSL_set_fd() failed";
        SSL_free_ptr(ssl);
        return nullptr;
      }

      int r = SSL_connect_ptr(ssl);
      if (r <= 0)
      {
        error = "SSL_connect() failed";
        SSL_shutdown_ptr(ssl);
        SSL_free_ptr(ssl);
        return nullptr;
      }

      return ssl;
    }

    int Read(void* ssl, void* buf, int len)
    {
      return SSL_read_ptr(ssl, buf, len);
    }

    int Write(void* ssl, const void* buf, int len)
    {
      return SSL_write_ptr(ssl, buf, len);
    }

    void Shutdown(void* ssl)
    {
      if (!ssl)
        return;

      SSL_shutdown_ptr(ssl);
      SSL_free_ptr(ssl);
    }
  };
}

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
    << "  logmectl -p <port> [-i <ip>] [--ssl] <command...>\n\n"
    << "Run with \"help\" to see available commands.\n";
}

static bool ParseArgs(
  int argc
  , char** argv
  , std::string& ip
  , int& port
  , bool& useSsl
  , std::string& command
)
{
  ip = "127.0.0.1";
  port = 0;
  useSsl = false;

  std::vector<std::string> cmd;

  for (int i = 1; i < argc; ++i)
  {
    const char* a = argv[i];

    if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0)
    {
      return false;
    }

    if (strcmp(a, "-s") == 0 || strcmp(a, "--ssl") == 0)
    {
      useSsl = true;
      continue;
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
  bool useSsl = false;
  std::string cmd;

  if (!ParseArgs(argc, argv, ip, port, useSsl, cmd))
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

  ClientSslContext sslCtx;
  void* ssl = nullptr;

  if (useSsl)
  {
    std::string sslErr;

    if (!sslCtx.Init(sslErr))
    {
      std::cerr << "ERROR: " << sslErr << "\n";
      CloseSocket(s);
      return 1;
    }

    ssl = sslCtx.CreateSession((int)s, sslErr);
    if (!ssl)
    {
      std::cerr << "ERROR: " << sslErr << "\n";
      CloseSocket(s);
      return 1;
    }
  }

  int sent = 0;

  if (useSsl)
    sent = sslCtx.Write(ssl, cmd.c_str(), (int)cmd.size());
  else
    sent = send(s, cmd.c_str(), (int)cmd.size(), 0);

  if (sent <= 0)
  {
    std::cerr << "ERROR: send failed\n";

    if (useSsl)
      sslCtx.Shutdown(ssl);

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
    int n = 0;

    if (useSsl)
      n = sslCtx.Read(ssl, buf, (int)sizeof(buf));
    else
      n = (int)recv(s, buf, (int)sizeof(buf), 0);

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

  if (useSsl)
    sslCtx.Shutdown(ssl);

  CloseSocket(s);

#ifdef _WIN32
  WSACleanup();
#endif

  return 0;
}
