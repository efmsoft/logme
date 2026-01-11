#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <vector>

#ifndef _WIN32
#include <pthread.h>
#include <signal.h>
#endif

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
#include <unistd.h>
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

#ifndef _WIN32
  void ControlBlockSigPipeForThread()
  {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &set, nullptr);
  }
#endif

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

void Logger::FreeControlSsl()
{
  std::lock_guard guard(DataLock);

  if (ControlSsl)
  {
    delete ControlSsl;
    ControlSsl = nullptr;
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
      if (response.back() != '\n')
        response.push_back('\n');

      if (sslCtx)
        n = sslCtx->Write(ssl, response.c_str(), (int)response.size());
      else
      {
        int flags = 0;
#ifdef MSG_NOSIGNAL
        flags |= MSG_NOSIGNAL;
#endif
        n = (int)send(
          socket
          , response.c_str()
          , (int)response.size()
          , flags
        );
      }

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
  auto SplitRemainderAfterWords = [](const std::string& s, int words) -> std::string
  {
    int w = 0;
    bool inWord = false;

    for (size_t i = 0; i < s.size(); ++i)
    {
      unsigned char ch = (unsigned char)s[i];
      bool ws = (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');

      if (!ws)
      {
        if (!inWord)
        {
          inWord = true;
          ++w;
          if (w == words + 1)
            return s.substr(i);
        }
      }
      else
      {
        inWord = false;
      }
    }

    return std::string();
  };

  auto JsonEscape = [](const std::string& s) -> std::string
  {
    std::string out;
    out.reserve(s.size() + 16);

    for (unsigned char ch : s)
    {
      switch (ch)
      {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
          if (ch < 0x20)
          {
            char b[7];
            std::snprintf(b, sizeof(b), "\\u%04x", (unsigned int)ch);
            out += b;
          }
          else
          {
            out.push_back((char)ch);
          }
          break;
      }
    }

    return out;
  };

  auto ControlInternal = [&](const std::string& cmd) -> std::string
  {
    std::string k(cmd);
    std::transform(k.begin(), k.end(), k.begin(), ::tolower);

    StringArray items;
    size_t n = WordSplit(k, items);

    std::string response;
    if (ControlExtension)
    {
      if (ControlExtension(cmd, response))
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

    return std::string("error: unsupported command: ") + (n ? items[0] : "<empty>");
  };

  std::string k(command);
  std::transform(k.begin(), k.end(), k.begin(), ::tolower);

  StringArray items;
  size_t n = WordSplit(k, items);

  if (n >= 2 && items[0] == "format")
  {
    if (items[1] == "json")
    {
      std::string remainder = SplitRemainderAfterWords(command, 2);
      std::string text = ControlInternal(remainder);
      bool ok = !(text.rfind("error:", 0) == 0);

      std::string remainderLower = remainder;
      std::transform(remainderLower.begin(), remainderLower.end(), remainderLower.begin(), ::tolower);

      StringArray remainderItems;
      (void)WordSplit(remainderLower, remainderItems);

      auto FirstLine = [&](const std::string& s) -> std::string
      {
        size_t e = s.find('\n');
        if (e == std::string::npos)
          return s;
        return s.substr(0, e);
      };

      auto Trim = [&](std::string& s)
      {
        while (!s.empty() && (s[0] == ' ' || s[0] == '\t' || s[0] == '\r' || s[0] == '\n'))
          s.erase(0, 1);
        while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n'))
          s.pop_back();
      };

      auto SplitLines = [&](const std::string& s, std::vector<std::string>& out)
      {
        out.clear();
        size_t p = 0;
        for (;;)
        {
          size_t e = s.find('\n', p);
          std::string line;
          if (e == std::string::npos)
            line = s.substr(p);
          else
            line = s.substr(p, e - p);

          if (!line.empty() && line.back() == '\r')
            line.pop_back();

          out.push_back(line);

          if (e == std::string::npos)
            break;
          p = e + 1;
        }
      };

      auto JsonBoolField = [&](std::string& out, const char* name, bool value, bool& first)
      {
        if (!first)
          out += ',';
        first = false;
        out += '"';
        out += name;
        out += "\":";
        out += value ? "true" : "false";
      };

      auto JsonStringField = [&](std::string& out, const char* name, const std::string& value, bool& first)
      {
        if (!first)
          out += ',';
        first = false;
        out += '"';
        out += name;
        out += "\":\"";
        out += JsonEscape(value);
        out += '"';
      };

      auto JsonIntField = [&](std::string& out, const char* name, uint64_t value, bool& first)
      {
        if (!first)
          out += ',';
        first = false;
        out += '"';
        out += name;
        out += "\":";
        out += std::to_string(value);
      };

      auto JsonStringArrayField = [&](std::string& out, const char* name, const std::vector<std::string>& values, bool& first)
      {
        if (!first)
          out += ',';
        first = false;
        out += '"';
        out += name;
        out += "\":[";
        for (size_t i = 0; i < values.size(); ++i)
        {
          if (i)
            out += ',';
          out += '"';
          out += JsonEscape(values[i]);
          out += '"';
        }
        out += ']';
      };

      std::string data;
      data.reserve(text.size() + 128);
      data += "{";
      bool dataFirst = true;

      if (ok && !remainderItems.empty())
      {
        const std::string& cmdName = remainderItems[0];

        if (cmdName == "subsystem")
        {
          std::vector<std::string> lines;
          SplitLines(text, lines);

          bool hasBlock = false;
          bool block = false;
          bool hasReported = false;
          std::vector<std::string> subs;

          for (size_t i = 0; i < lines.size(); ++i)
          {
            std::string line = lines[i];
            Trim(line);

            if (line.rfind("BlockReportedSubsystems:", 0) == 0)
            {
              std::string v = line.substr(std::strlen("BlockReportedSubsystems:"));
              Trim(v);
              hasBlock = true;
              block = (v == "true");
              continue;
            }

            if (line == "Reported subsystems: none")
            {
              hasReported = true;
              subs.clear();
              continue;
            }

            if (line == "Reported subsystems:")
            {
              hasReported = true;
              subs.clear();
              for (size_t j = i + 1; j < lines.size(); ++j)
              {
                std::string s = lines[j];
                Trim(s);
                if (s.empty())
                  continue;
                subs.push_back(s);
              }
              break;
            }
          }

          if (hasBlock)
            JsonBoolField(data, "blockReportedSubsystems", block, dataFirst);
          if (hasReported)
            JsonStringArrayField(data, "reportedSubsystems", subs, dataFirst);
        }
        else if (cmdName == "list")
        {
          std::vector<std::string> lines;
          SplitLines(text, lines);

          std::vector<std::string> channels;
          for (auto& l : lines)
          {
            std::string line = l;
            Trim(line);
            if (line.empty())
              continue;
            if (line == "<default>")
              channels.push_back("");
            else
              channels.push_back(line);
          }

          JsonStringArrayField(data, "channels", channels, dataFirst);
        }
        else if (cmdName == "flags")
        {
          std::string line = FirstLine(text);
          Trim(line);

          if (line.rfind("Flags:", 0) == 0)
          {
            std::string rest = line.substr(std::strlen("Flags:"));
            Trim(rest);

            size_t sp = rest.find(' ');
            std::string hex = rest;
            std::string names;
            if (sp != std::string::npos)
            {
              hex = rest.substr(0, sp);
              names = rest.substr(sp + 1);
            }

            Trim(hex);
            Trim(names);

            uint64_t value = 0;
            if (!hex.empty())
              value = (uint64_t)std::strtoull(hex.c_str(), nullptr, 0);

            std::vector<std::string> arr;
            if (!names.empty())
            {
              StringArray tokens;
              WordSplit(names, tokens);
              for (auto& t : tokens)
              {
                if (!t.empty())
                  arr.push_back(t);
              }
            }

            JsonIntField(data, "value", value, dataFirst);
            JsonStringArrayField(data, "names", arr, dataFirst);
          }
        }
        else if (cmdName == "level")
        {
          std::string line = FirstLine(text);
          Trim(line);
          if (line.rfind("Level:", 0) == 0)
          {
            std::string v = line.substr(std::strlen("Level:"));
            Trim(v);
            if (!v.empty())
              JsonStringField(data, "level", v, dataFirst);
          }
        }
      }

      JsonStringField(data, "text", text, dataFirst);
      data += "}";
      std::string json;

      json.reserve(text.size() + 64);
      json += "{";
      json += "\"ok\":";
      json += ok ? "true" : "false";
      json += ",\"error\":";

      if (ok)
      {
        json += "null";
      }
      else
      {
        std::string err = text;
        if (err.rfind("error:", 0) == 0)
        {
          err.erase(0, 6);
          while (!err.empty() && (err[0] == ' ' || err[0] == '\t'))
            err.erase(0, 1);
        }
        json += "\"" + JsonEscape(err) + "\"";
      }

      json += ",\"data\":";
      json += data;
      json += "}";
      return json;
    }

    if (items[1] == "text" || items[1] == "plain" || items[1] == "plain/text")
    {
      std::string remainder = SplitRemainderAfterWords(command, 2);
      return ControlInternal(remainder);
    }

    return "error: invalid format";
  }

  return ControlInternal(command);
}