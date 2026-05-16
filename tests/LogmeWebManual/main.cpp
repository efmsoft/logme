#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#endif

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <Logme/Backend/BufferBackend.h>
#include <Logme/Backend/ConsoleBackend.h>
#include <Logme/Backend/FileBackend.h>
#include <Logme/Logme.h>
#include <Logme/Ssl.h>

namespace
{
struct Options
{
  bool UseSsl = false;
  int Port = 7791;
  std::string Password;
  bool UseObfuscation = false;
  std::string CertFile;
  std::string KeyFile;
  bool GeneratedCertificate = false;
  int IntervalMs = 1000;
};

static void PrintUsage(const char* exe)
{
  std::cout
    << "logmeweb manual test host\n"
    << "\n"
    << "Usage:\n"
    << "  " << exe << " [options]\n"
    << "\n"
    << "Options:\n"
    << "  --http <port>             Start HTTP control server on port. Default: 7791.\n"
    << "  --https <port>            Start HTTPS control server on port.\n"
    << "  --ssl                     Alias for --https 7792.\n"
    << "  --password <text>         Require password for control commands.\n"
    << "  --pass <text>             Alias for --password.\n"
    << "  --obfuscation on|off      Enable or disable log obfuscation. Default: off.\n"
    << "  --cert <file>             TLS certificate file for HTTPS.\n"
    << "  --key <file>              TLS private key file for HTTPS.\n"
    << "  --interval-ms <value>     Log interval in milliseconds. Default: 1000.\n"
    << "  --help                    Show this help.\n"
    << "\n"
    << "Manual check examples:\n"
    << "  " << exe << " --http 7791\n"
    << "  " << exe << " --http 7791 --password test\n"
    << "  " << exe << " --https 7792\n"
    << "  " << exe << " --http 7791 --obfuscation on\n"
    << std::endl;
}

static bool ReadInt(
  const char* text
  , int& value
)
{
  char* end = nullptr;
  long parsed = std::strtol(text, &end, 10);

  if (end == text || *end != '\0')
    return false;

  if (parsed <= 0 || parsed > 65535)
    return false;

  value = static_cast<int>(parsed);
  return true;
}

static bool ParseOnOff(
  const std::string& text
  , bool& value
)
{
  if (text == "on" || text == "true" || text == "1")
  {
    value = true;
    return true;
  }

  if (text == "off" || text == "false" || text == "0")
  {
    value = false;
    return true;
  }

  return false;
}

static bool ParseArgs(
  int argc
  , char** argv
  , Options& options
)
{
  for (int i = 1; i < argc; ++i)
  {
    const std::string arg = argv[i];

    if (arg == "--help" || arg == "help" || arg == "-h")
      return false;

    if (arg == "--http" && i + 1 < argc)
    {
      options.UseSsl = false;
      if (!ReadInt(argv[++i], options.Port))
      {
        std::cerr << "Invalid HTTP port" << std::endl;
        return false;
      }
      continue;
    }

    if (arg == "--https" && i + 1 < argc)
    {
      options.UseSsl = true;
      if (!ReadInt(argv[++i], options.Port))
      {
        std::cerr << "Invalid HTTPS port" << std::endl;
        return false;
      }
      continue;
    }

    if (arg == "--ssl")
    {
      options.UseSsl = true;
      options.Port = 7792;
      continue;
    }

    if ((arg == "--password" || arg == "--pass") && i + 1 < argc)
    {
      options.Password = argv[++i];
      continue;
    }

    if (arg == "--obfuscation" && i + 1 < argc)
    {
      if (!ParseOnOff(argv[++i], options.UseObfuscation))
      {
        std::cerr << "Invalid obfuscation value" << std::endl;
        return false;
      }
      continue;
    }

    if (arg == "--cert" && i + 1 < argc)
    {
      options.CertFile = argv[++i];
      continue;
    }

    if (arg == "--key" && i + 1 < argc)
    {
      options.KeyFile = argv[++i];
      continue;
    }

    if (arg == "--interval-ms" && i + 1 < argc)
    {
      if (!ReadInt(argv[++i], options.IntervalMs))
      {
        std::cerr << "Invalid interval" << std::endl;
        return false;
      }
      continue;
    }

    std::cerr << "Unknown option: " << arg << std::endl;
    return false;
  }

  return true;
}

static Logme::ChannelPtr CreateChannel(const char* name)
{
  return Logme::Instance->CreateChannel(Logme::ID{ name }, Logme::OutputFlags(), Logme::LEVEL_DEBUG);
}

static Logme::ChannelPtr CreateLinkedChannel(const char* name)
{
  auto channel = CreateChannel(name);
  channel->AddLink(::CH);
  return channel;
}

static void ConfigureDemoBackends(const Logme::ChannelPtr& channel)
{
  auto console = std::make_shared<Logme::ConsoleBackend>(channel);
  console->SetAsync(true);
  channel->AddBackend(console);

  auto file = std::make_shared<Logme::FileBackend>(channel);
  file->SetAppend(true);
  file->SetMaxSize(1024 * 1024);
  if (file->CreateLog("logmeweb-manual-webtest.log"))
    channel->AddBackend(file);

  auto buffer = std::make_shared<Logme::BufferBackend>(channel);
  channel->AddBackend(buffer);
}

static void ConfigureDemoLogger(
  Logme::ChannelPtr& webtest
  , Logme::ChannelPtr& quiet
  , Logme::ChannelPtr& errtest
)
{
  webtest = CreateChannel("WEBTEST");
  quiet = CreateLinkedChannel("QUIET");
  errtest = CreateLinkedChannel("ERRTEST");

  ConfigureDemoBackends(webtest);
  quiet->SetEnabled(false);

  Logme::Instance->AddBlockedSubsystem(Logme::SID::Build("BLOCKED"));
  Logme::Instance->AddAllowedSubsystem(Logme::SID::Build("NET"));
  Logme::Instance->AddAllowedSubsystem(Logme::SID::Build("HTTP"));
  Logme::Instance->AddAllowedSubsystem(Logme::SID::Build("AUTH"));
  Logme::Instance->AddAllowedSubsystem(Logme::SID::Build("CACHE"));
}

static void ConfigureObfuscation(bool enable)
{
  if (!enable)
  {
    Logme::Instance->SetObfuscationKey(nullptr);
    return;
  }

  ObfKey key{};
  for (size_t i = 0; i < LOGOBF_KEY_BYTES; ++i)
    key.Bytes[i] = static_cast<uint8_t>(i * 17 + 3);

  Logme::Instance->SetObfuscationKey(&key);
}

static void PrintInstructions(const Options& options)
{
  const char* protocol = options.UseSsl ? "https" : "http";

  std::cout
    << "logmeweb manual test host is running.\n"
    << "\n"
    << "Open logmeweb and connect to:\n"
    << "  " << protocol << "://127.0.0.1:" << options.Port << "/\n"
    << "\n";

  if (options.UseSsl && options.GeneratedCertificate)
  {
    std::cout
      << "Generated local self-signed certificate files:\n"
      << "  certificate: " << options.CertFile << "\n"
      << "  private key: " << options.KeyFile << "\n"
      << "These files are generated locally for the manual test host and should not be committed.\n"
      << "\n";
  }

  if (!options.Password.empty())
  {
    std::cout
      << "Password: " << options.Password << "\n"
      << "\n";
  }

  std::cout
    << "Suggested manual checks:\n"
    << "  1. Open Overview and verify counters change.\n"
    << "  2. Open Channels and enable/disable WEBTEST and QUIET.\n"
    << "  3. Add another FileBackend and verify the file grows.\n"
    << "  4. Open Backends and verify ConsoleBackend, FileBackend and BufferBackend details.\n"
    << "  5. Open Trace points and enable/disable WorkerLoop or ErrorPath.\n"
    << "  6. Use Manual command: help, overview, channels, trace list, trace stats.\n"
    << "  7. Restart with --password and verify authentication.\n"
    << "  8. Restart with --https and verify HTTPS connection.\n"
    << "  9. Restart with --obfuscation on/off and verify both modes.\n"
    << "\n"
    << "Press ENTER to stop.\n"
    << std::endl;
}


#ifdef _WIN32
static bool FileExists(const std::string& path)
{
  DWORD attrs = GetFileAttributesA(path.c_str());
  return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static bool DirectoryExists(const std::string& path)
{
  DWORD attrs = GetFileAttributesA(path.c_str());
  return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static std::string JoinPath(
  const std::string& dir
  , const char* name
)
{
  if (dir.empty())
    return name;

  char last = dir[dir.size() - 1];
  if (last == '\\' || last == '/')
    return dir + name;

  return dir + "\\" + name;
}

static std::string ParentDirectory(const std::string& path)
{
  size_t pos = path.find_last_of("\\/");
  if (pos == std::string::npos)
    return std::string();

  return path.substr(0, pos);
}

static std::string GetExecutableDirectory()
{
  char filename[MAX_PATH] = {0};
  DWORD len = GetModuleFileNameA(nullptr, filename, MAX_PATH);
  if (len == 0 || len >= MAX_PATH)
    return std::string();

  return ParentDirectory(filename);
}

static bool ContainsOpenSslRuntime(const std::string& dir)
{
  static const char* SSL_NAMES[] =
  {
#if defined(_WIN64)
    "libssl-3-x64.dll",
    "libssl-1_1-x64.dll",
#endif
    "libssl-3.dll",
    "libssl-1_1.dll",
    "libssl.dll",
    nullptr
  };

  static const char* CRYPTO_NAMES[] =
  {
#if defined(_WIN64)
    "libcrypto-3-x64.dll",
    "libcrypto-1_1-x64.dll",
#endif
    "libcrypto-3.dll",
    "libcrypto-1_1.dll",
    "libcrypto.dll",
    nullptr
  };

  bool hasSsl = false;
  bool hasCrypto = false;

  for (const char** name = SSL_NAMES; *name; ++name)
  {
    if (FileExists(JoinPath(dir, *name)))
    {
      hasSsl = true;
      break;
    }
  }

  for (const char** name = CRYPTO_NAMES; *name; ++name)
  {
    if (FileExists(JoinPath(dir, *name)))
    {
      hasCrypto = true;
      break;
    }
  }

  return hasSsl && hasCrypto;
}

static void AddCandidateOpenSslDirs(
  std::vector<std::string>& dirs
  , const std::string& root
)
{
  if (root.empty())
    return;

  static const char* TRIPLETS[] =
  {
    "x64-windows",
    "x64-windows-static-md",
    "x64-windows-static",
    nullptr
  };

  for (const char** triplet = TRIPLETS; *triplet; ++triplet)
  {
    dirs.push_back(root + "\\installed\\" + *triplet + "\\bin");
    dirs.push_back(root + "\\installed\\" + *triplet + "\\debug\\bin");
  }
}

static std::vector<std::string> GetOpenSslRuntimeDirs()
{
  std::vector<std::string> dirs;

  std::string exeDir = GetExecutableDirectory();
  if (!exeDir.empty())
  {
    dirs.push_back(exeDir);
    dirs.push_back(ParentDirectory(exeDir));
  }

  const char* vcpkgRoot = std::getenv("VCPKG_ROOT");
  if (vcpkgRoot && *vcpkgRoot)
    AddCandidateOpenSslDirs(dirs, vcpkgRoot);

  AddCandidateOpenSslDirs(dirs, "C:\\dev\\vcpkg");
  return dirs;
}

static void ConfigureOpenSslDllDirectory()
{
  std::vector<std::string> dirs = GetOpenSslRuntimeDirs();

  for (const std::string& dir : dirs)
  {
    if (!DirectoryExists(dir))
      continue;

    if (!ContainsOpenSslRuntime(dir))
      continue;

    SetDllDirectoryA(dir.c_str());
    std::cout << "Using OpenSSL runtime directory: " << dir << std::endl;
    return;
  }
}

static void PrintOpenSslRuntimeDiagnostics()
{
  std::vector<std::string> dirs = GetOpenSslRuntimeDirs();
  bool found = false;

  for (const std::string& dir : dirs)
  {
    if (!DirectoryExists(dir))
      continue;

    if (!ContainsOpenSslRuntime(dir))
      continue;

    found = true;
    std::cerr << "OpenSSL runtime DLLs found in: " << dir << std::endl;
  }

  if (found)
    return;

  std::cerr
    << "OpenSSL runtime DLLs were not found near the executable or in known vcpkg locations.\n"
    << "For HTTPS mode LogmeWebManual needs both libssl*.dll and libcrypto*.dll.\n"
    << "Reconfigure/rebuild the target so CMake can copy the OpenSSL runtime DLLs next to LogmeWebManual.exe."
    << std::endl;
}
#endif

static bool ConfigureSsl(
  Options& options
  , X509*& cert
  , EVP_PKEY*& key
)
{
  cert = nullptr;
  key = nullptr;

  if (!options.UseSsl)
    return true;

  if (options.CertFile.empty() != options.KeyFile.empty())
  {
    std::cerr
      << "ERROR: --cert and --key must be specified together.\n"
      << "       Omit both options to let the manual test host generate a local self-signed certificate."
      << std::endl;
    return false;
  }

  if (options.CertFile.empty())
  {
    std::ostringstream certName;
    certName << "logmeweb_manual_" << options.Port << "_cert.pem";

    std::ostringstream keyName;
    keyName << "logmeweb_manual_" << options.Port << "_key.pem";

    options.CertFile = certName.str();
    options.KeyFile = keyName.str();
    options.GeneratedCertificate = true;

    std::string genError;
    if (!Logme::GenerateSelfSignedCertificateFiles(
      options.CertFile.c_str()
      , options.KeyFile.c_str()
      , genError
    ))
    {
      std::cerr << "ERROR: unable to generate default TLS certificate: " << genError << std::endl;
      return false;
    }
  }

  std::string error;
  if (!Logme::LoadSslCertificateFromFile(options.CertFile.c_str(), &cert, error))
  {
    std::cerr << "ERROR: " << error << std::endl;
    return false;
  }

  if (!Logme::LoadSslPrivateKeyFromFile(options.KeyFile.c_str(), &key, error))
  {
    Logme::FreeSslCertificate(cert);
    cert = nullptr;
    std::cerr << "ERROR: " << error << std::endl;
    return false;
  }

  return true;
}

static bool StartControl(
  const Options& options
  , X509* cert
  , EVP_PKEY* key
)
{
  Logme::ControlConfig config{};
  config.Enable = true;
  config.Port = options.Port;
  config.Interface = 0;
  config.Cert = cert;
  config.Key = key;
  config.Password = options.Password.empty() ? nullptr : options.Password.c_str();
  config.DiscoveryEnable = true;
  config.DiscoveryNamePrefix = "logmeweb-manual";

  return Logme::Instance->StartControlServer(config);
}

static void WorkerLoop(
  std::atomic<bool>& stop
  , const Options& options
  , const Logme::ChannelPtr& webtest
  , const Logme::ChannelPtr& quiet
  , const Logme::ChannelPtr& errtest
)
{
  Logme::SID net = Logme::SID::Build("NET");
  Logme::SID http = Logme::SID::Build("HTTP");
  Logme::SID auth = Logme::SID::Build("AUTH");
  Logme::SID cache = Logme::SID::Build("CACHE");
  Logme::SID blocked = Logme::SID::Build("BLOCKED");

  int counter = 0;
  while (!stop.load())
  {
    LogmeI_TPt(webtest, net, "WorkerLoop visible info record %d", counter);
    LogmeD_TPt(webtest, http, "HttpRequest debug record %d", counter);
    LogmeW(webtest, cache, "Cache warning record %d", counter);
    LogmeI(quiet, auth, "QUIET channel record %d", counter);
    LogmeE_TPt(errtest, auth, "ErrorPath error record %d", counter);
    LogmeI(webtest, blocked, "Blocked subsystem record %d", counter);

    ++counter;
    std::this_thread::sleep_for(std::chrono::milliseconds(options.IntervalMs));
  }
}
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
  WSADATA wsa = {0};
  int rc = WSAStartup(MAKEWORD(2, 2), &wsa);
  if (rc != 0)
  {
    std::cerr << "WSAStartup failed: " << rc << std::endl;
    return 1;
  }
#endif

  Options options;
  if (!ParseArgs(argc, argv, options))
  {
    PrintUsage(argv[0]);
#ifdef _WIN32
    WSACleanup();
#endif
    return 1;
  }

  ConfigureObfuscation(options.UseObfuscation);

#ifdef _WIN32
  if (options.UseSsl)
    ConfigureOpenSslDllDirectory();
#endif

  X509* cert = nullptr;
  EVP_PKEY* key = nullptr;
  if (!ConfigureSsl(options, cert, key))
  {
#ifdef _WIN32
    WSACleanup();
#endif
    return 2;
  }

  Logme::ChannelPtr webtest;
  Logme::ChannelPtr quiet;
  Logme::ChannelPtr errtest;
  ConfigureDemoLogger(webtest, quiet, errtest);

  if (!StartControl(options, cert, key))
  {
    std::cerr
      << "Failed to start control server on "
      << (options.UseSsl ? "HTTPS" : "HTTP")
      << " port " << options.Port << ".\n";

    if (options.UseSsl)
    {
#ifdef _WIN32
      PrintOpenSslRuntimeDiagnostics();
#endif
      std::cerr
        << "For HTTPS this can also mean that OpenSSL runtime initialization failed.\n";
    }

    std::cerr
      << "Also check that the port is not already used by another process."
      << std::endl;

    if (key)
      Logme::FreeSslPrivateKey(key);
    if (cert)
      Logme::FreeSslCertificate(cert);
#ifdef _WIN32
    WSACleanup();
#endif
    return 3;
  }

  PrintInstructions(options);

  std::atomic<bool> stop{ false };
  std::thread worker(WorkerLoop, std::ref(stop), std::cref(options), webtest, quiet, errtest);

  std::string line;
  std::getline(std::cin, line);

  stop.store(true);
  worker.join();

  if (key)
    Logme::FreeSslPrivateKey(key);
  if (cert)
    Logme::FreeSslCertificate(cert);

#ifdef _WIN32
  WSACleanup();
#endif

  return 0;
}
