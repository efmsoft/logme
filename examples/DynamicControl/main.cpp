#ifdef _WIN32
#include <winsock2.h>
#endif

#include "Worker.h"

#include <iostream>
#include <string>
#include <cstring>

#include <fstream>

#include <Logme/Logger.h>
#include <Logme/Logme.h>
#include <Logme/Ssl.h>

static void PrintInstructions(
  int port
  , bool ssl
  , const std::string& pass
)
{
  const char* sslArg = ssl ? " --ssl" : "";
  std::string passArg;

  if (!pass.empty())
    passArg = " --pass " + pass;

  std::cout
    << "Dynamic control demo\n"
    << "\n"
    << "This example starts Logme control server and prints from two worker threads.\n"
    << "Use 'logmectl' (built with -DLOGME_BUILD_TOOLS=ON) from another console to change settings.\n"
    << "\n"
    << "Control server: 127.0.0.1:" << port << "\n"
    << "\n"
    << "Useful commands:\n"
    << "  logmectl" << sslArg << passArg << " -p " << port << " backend --channel ch1 --add console\n"
    << "  logmectl" << sslArg << passArg << " -p " << port << " subsystem --unblock-reported\n"
    << "  logmectl" << sslArg << passArg << " -p " << port << " subsystem --report s1\n"
    << "  logmectl" << sslArg << passArg << " -p " << port << " channel --disable ch1\n"
    << "  logmectl" << sslArg << passArg << " -p " << port << " backend --channel ch2 --add console\n"
    << "  logmectl" << sslArg << passArg << " -p " << port << " channel --enable ch1\n"
    << "\n";

  if (ssl)
  {
    std::cout << "Note: TLS is enabled. Use logmectl --ssl when connecting." << std::endl;
  }

  std::cout
    << "Press ENTER to stop.\n"
    << std::endl;
}

static bool ParseArgs(
  int argc
  , char** argv
  , bool& ssl
  , std::string& certFile
  , std::string& keyFile
  , std::string& pass
)
{
  ssl = false;
  pass.clear();
  for (int i = 1; i < argc; ++i)
  {
    const std::string a = argv[i];

    if (a == "--ssl")
    {
      ssl = true;
      continue;
    }

    if (a == "--cert" && i + 1 < argc)
    {
      certFile = argv[++i];
      continue;
    }

    if (a == "--key" && i + 1 < argc)
    {
      keyFile = argv[++i];
      continue;
    }

    if (a == "--pass" && i + 1 < argc)
    {
      pass = argv[++i];
      continue;
    }

    if (a == "--help" || a == "help")
      return false;
  }

  return true;
}

static void PrintUsage(const char* exe)
{
  std::cout
    << "Usage:\n"
    << "  " << exe << " [--ssl [--cert file] [--key file]] [--pass password]\n"
    << std::endl;
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

  bool ssl = false;
  std::string certFile;
  std::string keyFile;
  std::string pass;

  if (!ParseArgs(argc, argv, ssl, certFile, keyFile, pass))
  {
    PrintUsage(argv[0]);
    return 1;
  }

  const int port = 7791;
  Logme::ControlConfig cfg{};
  cfg.Enable = true;
  cfg.Port = port;
  cfg.Interface = 0;
  cfg.Password = pass.empty() ? nullptr : pass.c_str();

  X509* cert = nullptr;
  EVP_PKEY* key = nullptr;

  if (ssl)
  {
    if (certFile.empty() && keyFile.empty())
    {
      certFile = "dynamiccontrol_demo_cert.pem";
      keyFile = "dynamiccontrol_demo_key.pem";

      std::string genError;
      if (!Logme::GenerateSelfSignedCertificateFiles(certFile.c_str(), keyFile.c_str(), genError))
      {
        std::cerr << "ERROR: " << genError << std::endl;
        return 2;
      }

      std::cout
        << "Demo TLS certificate generated:\n"
        << "  cert: " << certFile << "\n"
        << "  key : " << keyFile << "\n"
        << std::endl;
    }

    std::string error;

    if (!Logme::LoadSslCertificateFromFile(certFile.c_str(), &cert, error))
    {
      std::cerr << "ERROR: " << error << std::endl;
      return 3;
    }

    if (!Logme::LoadSslPrivateKeyFromFile(keyFile.c_str(), &key, error))
    {
      Logme::FreeSslCertificate(cert);
      std::cerr << "ERROR: " << error << std::endl;
      return 4;
    }

    cfg.Cert = cert;
    cfg.Key = key;
  }

  if (!Logme::Instance->StartControlServer(cfg))
  {
    std::cerr << "Failed to start control server" << std::endl;

#ifdef _WIN32
  WSACleanup();
#endif
    return 2;
  }

  PrintInstructions(port, ssl, pass);

  Worker w1("t1", "ch1", "s1", "s2");
  Worker w2("t2", "ch2", "s1", "s2");

  w1.Start();
  w2.Start();

  std::string line;
  std::getline(std::cin, line);

  w1.Stop();
  w2.Stop();


  if (key)
    Logme::FreeSslPrivateKey(key);
  if (cert)
    Logme::FreeSslCertificate(cert);

#ifdef _WIN32
  WSACleanup();
#endif

  return 0;
}
