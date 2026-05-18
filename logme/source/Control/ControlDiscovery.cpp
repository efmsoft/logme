#include "ControlDiscovery.h"
#include <Logme/version.h>

#include <cstddef>
#include <cstring>
#include <sstream>

#include <Logme/Logme.h>

#ifdef _WIN32
#include <Windows.h>
#ifdef _MSC_VER
#pragma comment(lib, "Advapi32.lib")
#endif
#else
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace Logme
{
#ifdef _WIN32
  static unsigned GetProcessIdForDiscovery()
  {
    return (unsigned)::GetCurrentProcessId();
  }

  static std::string GetDefaultDiscoveryPrefix()
  {
    return "\\\\.\\pipe\\Global\\logme-discovery-";
  }
#else
  static unsigned GetProcessIdForDiscovery()
  {
    return (unsigned)getpid();
  }

  static std::string GetDefaultDiscoveryPrefix()
  {
#ifdef __linux__
    return "logme-discovery-";
#else
    return "/tmp/logme-discovery-";
#endif
  }
#endif

  static std::string JsonEscape(const std::string& text)
  {
    std::string result;
    result.reserve(text.size() + 8);

    for (char ch : text)
    {
      switch (ch)
      {
        case '\\':
          result += "\\\\";
          break;
        case '"':
          result += "\\\"";
          break;
        case '\n':
          result += "\\n";
          break;
        case '\r':
          result += "\\r";
          break;
        case '\t':
          result += "\\t";
          break;
        default:
          result += ch;
          break;
      }
    }

    return result;
  }

  static std::string GetProcessNameForDiscovery()
  {
#ifdef _WIN32
    char path[MAX_PATH]{};
    DWORD length = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (!length)
      return "";

    std::string value(path, length);
#else
    char path[1024]{};
    ssize_t length = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (length <= 0)
      return "";

    std::string value(path, (size_t)length);
#endif

    size_t pos = value.find_last_of("/\\");
    if (pos == std::string::npos)
      return value;

    return value.substr(pos + 1);
  }

  ControlDiscovery::ControlDiscovery(const ControlConfig& config)
    : Config(config)
    , Stopped(false)
  {
    std::string prefix = Config.DiscoveryNamePrefix
      ? Config.DiscoveryNamePrefix
      : GetDefaultDiscoveryPrefix();

#ifdef _WIN32
    EndpointName = prefix + std::to_string(GetProcessIdForDiscovery());
#else
#ifdef __linux__
    if (!prefix.empty() && prefix[0] == '/')
      EndpointName = prefix + std::to_string(GetProcessIdForDiscovery());
    else
      EndpointName = prefix + std::to_string(GetProcessIdForDiscovery());
#else
    EndpointName = prefix + std::to_string(GetProcessIdForDiscovery()) + ".sock";
#endif
#endif
  }

  ControlDiscovery::~ControlDiscovery()
  {
    Stop();
  }

  bool ControlDiscovery::Start()
  {
    Stopped = false;

    try
    {
      Worker = std::thread(&ControlDiscovery::WorkerFunc, this);
    }
    catch (...)
    {
      return false;
    }

    return true;
  }

  void ControlDiscovery::Stop()
  {
    bool alreadyStopped = Stopped.exchange(true);
    if (alreadyStopped)
      return;

#ifdef _WIN32
    HANDLE pipe = CreateFileA(
      EndpointName.c_str()
      , GENERIC_READ | GENERIC_WRITE
      , 0
      , nullptr
      , OPEN_EXISTING
      , 0
      , nullptr
    );
    if (pipe != INVALID_HANDLE_VALUE)
      CloseHandle(pipe);
#else
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd >= 0)
    {
      sockaddr_un addr{};
      addr.sun_family = AF_UNIX;

#ifdef __linux__
      if (!EndpointName.empty() && EndpointName[0] != '/')
      {
        addr.sun_path[0] = '\0';
        std::strncpy(addr.sun_path + 1, EndpointName.c_str(), sizeof(addr.sun_path) - 2);
        size_t length = offsetof(sockaddr_un, sun_path) + 1 + EndpointName.size();
        connect(fd, (sockaddr*)&addr, (socklen_t)length);
      }
      else
#endif
      {
        std::strncpy(addr.sun_path, EndpointName.c_str(), sizeof(addr.sun_path) - 1);
        connect(fd, (sockaddr*)&addr, sizeof(addr));
      }

      close(fd);
    }
#endif

    if (Worker.joinable())
      Worker.join();

#ifndef _WIN32
#ifndef __linux__
    unlink(EndpointName.c_str());
#else
    if (!EndpointName.empty() && EndpointName[0] == '/')
      unlink(EndpointName.c_str());
#endif
#endif
  }

#ifdef _WIN32
  struct PipeSecurity
  {
    SECURITY_ATTRIBUTES Attributes{};
    SECURITY_DESCRIPTOR Descriptor{};
    PSID SystemSid = nullptr;
    PSID AdminSid = nullptr;
    PSID AuthenticatedUserSid = nullptr;
    PACL Acl = nullptr;
    bool Valid = false;

    PipeSecurity()
    {
      SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

      if (!AllocateAndInitializeSid(
        &ntAuthority
        , 1
        , SECURITY_LOCAL_SYSTEM_RID
        , 0
        , 0
        , 0
        , 0
        , 0
        , 0
        , 0
        , &SystemSid
      ))
        return;

      if (!AllocateAndInitializeSid(
        &ntAuthority
        , 2
        , SECURITY_BUILTIN_DOMAIN_RID
        , DOMAIN_ALIAS_RID_ADMINS
        , 0
        , 0
        , 0
        , 0
        , 0
        , 0
        , &AdminSid
      ))
        return;

      if (!AllocateAndInitializeSid(
        &ntAuthority
        , 1
        , SECURITY_AUTHENTICATED_USER_RID
        , 0
        , 0
        , 0
        , 0
        , 0
        , 0
        , 0
        , &AuthenticatedUserSid
      ))
        return;

      DWORD aclSize = sizeof(ACL)
        + GetLengthSid(SystemSid)
        + GetLengthSid(AdminSid)
        + GetLengthSid(AuthenticatedUserSid)
        + 3 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD));

      Acl = (PACL)LocalAlloc(LPTR, aclSize);
      if (!Acl)
        return;

      if (!InitializeAcl(Acl, aclSize, ACL_REVISION))
        return;

      if (!AddAccessAllowedAce(Acl, ACL_REVISION, GENERIC_ALL, SystemSid))
        return;

      if (!AddAccessAllowedAce(Acl, ACL_REVISION, GENERIC_ALL, AdminSid))
        return;

      // A service can create the discovery pipe under a service account.
      // logmeweb normally runs in an interactive user session, so the pipe
      // must explicitly allow local authenticated users to query metadata.
      if (!AddAccessAllowedAce(
        Acl
        , ACL_REVISION
        , GENERIC_READ | GENERIC_WRITE
        , AuthenticatedUserSid
      ))
        return;

      if (!InitializeSecurityDescriptor(
        &Descriptor
        , SECURITY_DESCRIPTOR_REVISION
      ))
        return;

      if (!SetSecurityDescriptorDacl(&Descriptor, TRUE, Acl, FALSE))
        return;

      Attributes.nLength = sizeof(Attributes);
      Attributes.lpSecurityDescriptor = &Descriptor;
      Attributes.bInheritHandle = FALSE;
      Valid = true;
    }

    ~PipeSecurity()
    {
      if (Acl)
        LocalFree(Acl);

      if (AuthenticatedUserSid)
        FreeSid(AuthenticatedUserSid);

      if (AdminSid)
        FreeSid(AdminSid);

      if (SystemSid)
        FreeSid(SystemSid);
    }

    SECURITY_ATTRIBUTES* Get()
    {
      return Valid ? &Attributes : nullptr;
    }
  };
#endif

  std::string ControlDiscovery::BuildResponse() const
  {
    std::ostringstream os;
    os
      << "{"
      << "\"version\":1,"
      << "\"protocolVersion\":1,"
      << "\"logmeVersion\":\"" << JsonEscape(LOGME_VERSION_STRING) << "\","
      << "\"pid\":" << GetProcessIdForDiscovery() << ","
      << "\"process\":\"" << JsonEscape(GetProcessNameForDiscovery()) << "\","
      << "\"control\":{"
      << "\"host\":\"127.0.0.1\","
      << "\"port\":" << Config.Port << ","
      << "\"protocol\":\"" << (Config.Cert && Config.Key ? "https" : "http") << "\""
      << "},"
      << "\"authRequired\":" << (Config.Password && *Config.Password ? "true" : "false")
      << "}\n";

    return os.str();
  }

#ifdef _WIN32
  void ControlDiscovery::WorkerFunc()
  {
    while (!Stopped)
    {
      PipeSecurity security;
      HANDLE pipe = CreateNamedPipeA(
        EndpointName.c_str()
        , PIPE_ACCESS_DUPLEX
        , PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT
        , 1
        , 65536
        , 65536
        , 0
        , security.Get()
      );

      if (pipe == INVALID_HANDLE_VALUE)
        return;

      BOOL connected = ConnectNamedPipe(pipe, nullptr)
        ? TRUE
        : (GetLastError() == ERROR_PIPE_CONNECTED);

      if (connected && !Stopped)
      {
        char buffer[64]{};
        DWORD readBytes = 0;
        ReadFile(pipe, buffer, sizeof(buffer) - 1, &readBytes, nullptr);

        if (std::strncmp(buffer, "INFO", 4) == 0)
        {
          std::string response = BuildResponse();
          DWORD written = 0;
          WriteFile(pipe, response.data(), (DWORD)response.size(), &written, nullptr);
        }
      }

      DisconnectNamedPipe(pipe);
      CloseHandle(pipe);
    }
  }
#else
  void ControlDiscovery::WorkerFunc()
  {
    int server = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server < 0)
      return;

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    socklen_t addressLength = sizeof(addr);

#ifdef __linux__
    if (!EndpointName.empty() && EndpointName[0] != '/')
    {
      addr.sun_path[0] = '\0';
      std::strncpy(addr.sun_path + 1, EndpointName.c_str(), sizeof(addr.sun_path) - 2);
      addressLength = (socklen_t)(offsetof(sockaddr_un, sun_path) + 1 + EndpointName.size());
    }
    else
#endif
    {
      unlink(EndpointName.c_str());
      std::strncpy(addr.sun_path, EndpointName.c_str(), sizeof(addr.sun_path) - 1);
#ifdef __linux__
      addressLength = sizeof(addr);
#endif
    }

    if (bind(server, (sockaddr*)&addr, addressLength) != 0)
    {
      close(server);
      return;
    }

    if (listen(server, 4) != 0)
    {
      close(server);
      return;
    }

    while (!Stopped)
    {
      int client = accept(server, nullptr, nullptr);
      if (client < 0)
        continue;

      if (!Stopped)
      {
        char buffer[64]{};
        ssize_t readBytes = read(client, buffer, sizeof(buffer) - 1);
        if (readBytes > 0 && std::strncmp(buffer, "INFO", 4) == 0)
        {
          std::string response = BuildResponse();
#ifdef MSG_NOSIGNAL
          send(client, response.data(), response.size(), MSG_NOSIGNAL);
#else
          send(client, response.data(), response.size(), 0);
#endif
        }
      }

      close(client);
    }

    close(server);
  }
#endif
}
