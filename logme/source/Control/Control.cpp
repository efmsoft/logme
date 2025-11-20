#include <algorithm>

#ifdef _WIN32
#include <io.h> 
#include <winsock2.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>
#include <windows.h>

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
#include <dlfcn.h>

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

namespace
{
#if defined(_WIN32)
  typedef HMODULE ControlLibHandle;
#else
  typedef void* ControlLibHandle;
#endif

  ControlLibHandle ControlLoadLibrary(const char* const* names)
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

  void* ControlGetSymbol(ControlLibHandle handle, const char* name)
  {
    if (!handle)
      return nullptr;

#if defined(_WIN32)
    return reinterpret_cast<void*>(GetProcAddress(handle, name));
#else
    return dlsym(handle, name);
#endif
  }

  void ControlCloseLibrary(ControlLibHandle handle)
  {
#if defined(_WIN32)
    if (handle)
      FreeLibrary(handle);
#else
    if (handle)
      dlclose(handle);
#endif
  }
}

namespace Logme
{
  struct ControlSslContext
  {
    ControlLibHandle LibSsl;
    ControlLibHandle LibCrypto;

    int (*SSL_library_init_ptr)();
    void (*SSL_load_error_strings_ptr)();
    void* (*TLS_server_method_ptr)();

    void* (*SSL_CTX_new_ptr)(void*);
    void (*SSL_CTX_free_ptr)(void*);
    int (*SSL_CTX_use_certificate_ptr)(void*, void*);
    int (*SSL_CTX_use_PrivateKey_ptr)(void*, void*);
    int (*SSL_CTX_check_private_key_ptr)(void*);

    void* (*SSL_new_ptr)(void*);
    void (*SSL_free_ptr)(void*);
    int (*SSL_set_fd_ptr)(void*, int);
    int (*SSL_accept_ptr)(void*);
    int (*SSL_read_ptr)(void*, void*, int);
    int (*SSL_write_ptr)(void*, const void*, int);
    int (*SSL_shutdown_ptr)(void*);

    void* Ctx;

    ControlSslContext()
      : LibSsl(nullptr)
      , LibCrypto(nullptr)
      , SSL_library_init_ptr(nullptr)
      , SSL_load_error_strings_ptr(nullptr)
      , TLS_server_method_ptr(nullptr)
      , SSL_CTX_new_ptr(nullptr)
      , SSL_CTX_free_ptr(nullptr)
      , SSL_CTX_use_certificate_ptr(nullptr)
      , SSL_CTX_use_PrivateKey_ptr(nullptr)
      , SSL_CTX_check_private_key_ptr(nullptr)
      , SSL_new_ptr(nullptr)
      , SSL_free_ptr(nullptr)
      , SSL_set_fd_ptr(nullptr)
      , SSL_accept_ptr(nullptr)
      , SSL_read_ptr(nullptr)
      , SSL_write_ptr(nullptr)
      , SSL_shutdown_ptr(nullptr)
      , Ctx(nullptr)
    {
    }

    ~ControlSslContext()
    {
      Cleanup();
    }

    void Cleanup()
    {
      if (Ctx && SSL_CTX_free_ptr)
        SSL_CTX_free_ptr(Ctx);

      Ctx = nullptr;

      ControlCloseLibrary(LibSsl);
      ControlCloseLibrary(LibCrypto);

      LibSsl = nullptr;
      LibCrypto = nullptr;
    }

    bool Init(
      X509* cert
      , EVP_PKEY* key
      , std::string& error
    )
    {
      if (!cert || !key)
      {
        error = "cert/key is null";
        return false;
      }

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

      LibSsl = ControlLoadLibrary(SSL_NAMES);
      if (!LibSsl)
      {
        error = "Unable to load libssl shared library";
        return false;
      }

      LibCrypto = ControlLoadLibrary(CRYPTO_NAMES);
      if (!LibCrypto)
      {
        error = "Unable to load libcrypto shared library";
        return false;
      }

#define LOAD_REQUIRED(ptr, lib, name)                                      \
      do                                                                   \
      {                                                                    \
        ptr = reinterpret_cast<decltype(ptr)>(ControlGetSymbol(lib, name));\
        if (!ptr)                                                          \
        {                                                                  \
          error = std::string("Missing OpenSSL symbol: ") + name;        \
          return false;                                                    \
        }                                                                  \
      } while (false)

      SSL_library_init_ptr = reinterpret_cast<decltype(SSL_library_init_ptr)>(
        ControlGetSymbol(LibSsl, "SSL_library_init"));
      SSL_load_error_strings_ptr = reinterpret_cast<decltype(SSL_load_error_strings_ptr)>(
        ControlGetSymbol(LibSsl, "SSL_load_error_strings"));

      LOAD_REQUIRED(TLS_server_method_ptr, LibSsl, "TLS_server_method");
      LOAD_REQUIRED(SSL_CTX_new_ptr, LibSsl, "SSL_CTX_new");
      LOAD_REQUIRED(SSL_CTX_free_ptr, LibSsl, "SSL_CTX_free");
      LOAD_REQUIRED(SSL_CTX_use_certificate_ptr, LibSsl, "SSL_CTX_use_certificate");
      LOAD_REQUIRED(SSL_CTX_use_PrivateKey_ptr, LibSsl, "SSL_CTX_use_PrivateKey");
      LOAD_REQUIRED(SSL_CTX_check_private_key_ptr, LibSsl, "SSL_CTX_check_private_key");
      LOAD_REQUIRED(SSL_new_ptr, LibSsl, "SSL_new");
      LOAD_REQUIRED(SSL_free_ptr, LibSsl, "SSL_free");
      LOAD_REQUIRED(SSL_set_fd_ptr, LibSsl, "SSL_set_fd");
      LOAD_REQUIRED(SSL_accept_ptr, LibSsl, "SSL_accept");
      LOAD_REQUIRED(SSL_read_ptr, LibSsl, "SSL_read");
      LOAD_REQUIRED(SSL_write_ptr, LibSsl, "SSL_write");
      LOAD_REQUIRED(SSL_shutdown_ptr, LibSsl, "SSL_shutdown");

#undef LOAD_REQUIRED

      if (SSL_library_init_ptr)
        SSL_library_init_ptr();

      if (SSL_load_error_strings_ptr)
        SSL_load_error_strings_ptr();

      void* method = TLS_server_method_ptr();
      if (!method)
      {
        error = "TLS_server_method() returned null";
        return false;
      }

      Ctx = SSL_CTX_new_ptr(method);
      if (!Ctx)
      {
        error = "SSL_CTX_new() failed";
        return false;
      }

      if (SSL_CTX_use_certificate_ptr(Ctx, cert) != 1)
      {
        error = "SSL_CTX_use_certificate() failed";
        return false;
      }

      if (SSL_CTX_use_PrivateKey_ptr(Ctx, key) != 1)
      {
        error = "SSL_CTX_use_PrivateKey() failed";
        return false;
      }

      if (SSL_CTX_check_private_key_ptr(Ctx) != 1)
      {
        error = "SSL_CTX_check_private_key() failed";
        return false;
      }

      return true;
    }

    void* CreateSession(int socket)
    {
      void* ssl = SSL_new_ptr(Ctx);
      if (!ssl)
      {
        LogmeE(CHINT, "SSL_new() failed for control connection");
        return nullptr;
      }

      if (SSL_set_fd_ptr(ssl, socket) != 1)
      {
        LogmeE(CHINT, "SSL_set_fd() failed for control connection");
        SSL_free_ptr(ssl);
        return nullptr;
      }

      int r = SSL_accept_ptr(ssl);
      if (r <= 0)
      {
        LogmeE(CHINT, "SSL_accept() failed for control connection");
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

void Logger::StopControlServer()
{
  ThreadPtr thread;

  if (ControlSocket != -1)
  {
    std::lock_guard guard(DataLock);

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

Result Logger::SetControlCertificate(
  X509* cert
  , EVP_PKEY* key
)
{
  if (!cert || !key)
    return RC_NO_ACCESS;

  bool running = false;
  ControlConfig cfg{};

  {
    std::lock_guard guard(DataLock);
    running = (ControlSocket != -1);
    cfg = ControlCfg;
  }

  std::unique_ptr<ControlSslContext> ctx(new ControlSslContext());
  std::string error;

  if (!ctx->Init(cert, key, error))
  {
    LogmeE(CHINT, "Unable to initialize OpenSSL for control server: %s", error.c_str());
    return RC_NO_ACCESS;
  }

  if (!running)
  {
    std::lock_guard guard(DataLock);

    if (ControlSsl)
    {
      delete ControlSsl;
      ControlSsl = nullptr;
    }

    ControlSsl = ctx.release();
    ControlCfg.Cert = cert;
    ControlCfg.Key = key;

    LogmeI(CHINT, "Control server configured to use SSL");
    return RC_NOERROR;
  }

  StopControlServer();

  {
    std::lock_guard guard(DataLock);

    if (ControlSsl)
    {
      delete ControlSsl;
      ControlSsl = nullptr;
    }

    ControlSsl = ctx.release();
    ControlCfg.Cert = cert;
    ControlCfg.Key = key;
  }

  if (!cfg.Enable)
    return RC_NOERROR;

  if (!StartControlServer(cfg))
  {
    LogmeE(CHINT, "Unable to restart control server with SSL");
    return RC_NO_ACCESS;
  }

  LogmeI(CHINT, "Control server restarted with SSL");
  return RC_NOERROR;
}


bool Logger::StartControlServer(const ControlConfig& c)
{
  StopControlServer();

  if (!c.Enable)
    return true;

  std::lock_guard guard(DataLock);

  ControlCfg = c;

  if (c.Cert && c.Key)
  {
    std::unique_ptr<ControlSslContext> ctx(new ControlSslContext());
    std::string error;

    if (!ctx->Init(c.Cert, c.Key, error))
    {
      LogmeE(CHINT, "Unable to initialize OpenSSL for control server: %s", error.c_str());
      return false;
    }

    if (ControlSsl)
    {
      delete ControlSsl;
      ControlSsl = nullptr;
    }

    ControlSsl = ctx.release();
    LogmeI(CHINT, "Control server configured to use SSL");
  }

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

      std::lock_guard guard(DataLock);
      ControlThreads[accept_sock] = ct;

      ControlThreads[accept_sock].Thread = std::make_shared<std::thread>(
        &Logger::ControlHandler
        , this
        , accept_sock
      );
    }

    // Remove items for closed connections
    std::lock_guard guard(DataLock);
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
    std::lock_guard guard(DataLock);
    for (auto& t : ControlThreads)
    {
      shutdown(t.first, SD_RECEIVE);
      closesocket(t.first);
    }
  }

  // Join connection threads
  std::lock_guard guard(DataLock);
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

  ControlSslContext* sslCtx = nullptr;
  {
    std::lock_guard guard(DataLock);
    sslCtx = ControlSsl;
  }

  void* ssl = nullptr;

  if (sslCtx)
  {
    ssl = sslCtx->CreateSession(socket);
    if (!ssl)
    {
      std::lock_guard guard(DataLock);
      auto it = ControlThreads.find(socket);
      if (it != ControlThreads.end())
        it->second.Stopped = true;

      return;
    }
  }
    
  for (;;)
  {
    char buff[4096]{};
    int n = -1;

    if (sslCtx)
      n = sslCtx->Read(ssl, buff, (int)sizeof(buff));
    else
      n = (int)recv(socket, buff, sizeof(buff), 0);

    if (sslCtx)
    {
      if (n <= 0)
        break;
    }
    else
    {
      if (n == -1)
        break;
    }

    std::string command(buff, n);
    std::string response = Control(command);

    if (!response.empty())
    {
      if (sslCtx)
        n = sslCtx->Write(ssl, response.c_str(), (int)response.size());
      else
        n = (int)send(socket, response.c_str(), (int)response.size(), 0);

      if (sslCtx)
      {
        if (n <= 0)
          break;
      }
      else
      {
        if (n == -1)
          break;
      }
    }
  }

  if (sslCtx)
    sslCtx->Shutdown(ssl);

  std::lock_guard guard(DataLock);
  auto it = ControlThreads.find(socket);
  if (it != ControlThreads.end())
    it->second.Stopped = true;
}

void Logger::SetControlExtension(Logme::TControlHandler handler)
{
  std::lock_guard guard(DataLock);
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