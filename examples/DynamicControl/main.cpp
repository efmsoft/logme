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

static void PrintInstructions(int port)
{
  std::cout
    << "Dynamic control demo\n"
    << "\n"
    << "This example starts Logme control server and prints from two worker threads.\n"
    << "Use 'logmectl' (built with -DLOGME_BUILD_TOOLS=ON) from another console to change settings.\n"
    << "\n"
    << "Control server: 127.0.0.1:" << port << "\n"
    << "\n"
    << "Useful commands:\n"
    << "  logmectl -p " << port << " help\n"
    << "  logmectl -p " << port << " list\n"
    << "  logmectl -p " << port << " channel t1\n"
    << "  logmectl -p " << port << " channel disable t1\n"
    << "  logmectl -p " << port << " channel enable t1\n"
    << "  logmectl -p " << port << " channel level t2 error\n"
    << "  logmectl -p " << port << " subsystem list\n"
    << "  logmectl -p " << port << " subsystem mode blacklist\n"
    << "  logmectl -p " << port << " subsystem add wrk1\n"
    << "  logmectl -p " << port << " subsystem add wrk2\n"
    << "\n"
    << "Press ENTER to stop.\n"
    << std::endl;
}


static const char DemoCertPem[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIDAjCCAeqgAwIBAgIUDL3gyhAm/e2JBKgHiwMsJWwfapYwDQYJKoZIhvcNAQEL\n"
"BQAwJDEiMCAGA1UEAwwZbG9nbWUgRHluYW1pY0NvbnRyb2wgZGVtbzAeFw0yNjAx\n"
"MDkxOTE0MTRaFw0zNjAxMDgxOTE0MTRaMCQxIjAgBgNVBAMMGWxvZ21lIER5bmFt\n"
"aWNDb250cm9sIGRlbW8wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDO\n"
"9WSP+sVIHJhKW2fCK94VXrcBHrjyk6Pj1amAa63FhFTgGz5Khk4elIGy5DcBSs3Y\n"
"V+D3N0Y36NgH+cyUfYQ6z7eSjtGJ1g7vS7dX4GA2eN9KurrZ51wVNUMrHjXz9YY7\n"
"K2Js9Sec8crsDdkn9YeV1NUKrFc5fJazpM3/cL9sWz1ylTtZjz57TM99ADmMn0xv\n"
"duWMBctkDOpSx5swoY+58jChe0m5v5U0/EnAQ9mtl8mylwmbfMlCWANkHDNJg1Fk\n"
"VBk12BIQFygB+mIh4fnxVfxpEeGY5OPi6jYAijVcTZiSIFZkubhKx386nMAJjRUv\n"
"8+02EixWev2Y/RovwNfjAgMBAAGjLDAqMAwGA1UdEwEB/wQCMAAwGgYDVR0RBBMw\n"
"EYIJbG9jYWxob3N0hwR/AAABMA0GCSqGSIb3DQEBCwUAA4IBAQAM5WEC60mZQzFw\n"
"XsG6fRMoK796KhsVH59pK4Kzz5G09bQ6m/rT+eJMg2wEvY1Sgesl+zIhBUUmm7jx\n"
"YfXR2C+8tRbv31Q/aOTMNesb2lyEqun5xbJFx4f179ld1gDa3O/q70IRc8gsHZW1\n"
"VWIH57z8z43x4UeQZt+/rZbNaeZe2T9F1OhzGbgg7q3Mqd3Mm5r4uKzRIDkhLyHm\n"
"xCnrflHIYppfNlYo5rJtRYbvhu98kFEL/joqNHeXNuIY3D/BBC8ncoVBIWf1CG5P\n"
"SjES43oQCl4vRDHNu3sfUCt6F51N34oRqDt+cL0MwYmSys+4G6I3yHAsaNcuiMJ3\n"
"EbE7Fxe7\n"
"-----END CERTIFICATE-----\n";

static const char DemoKeyPem[] =
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEpQIBAAKCAQEAzvVkj/rFSByYSltnwiveFV63AR648pOj49WpgGutxYRU4Bs+\n"
"SoZOHpSBsuQ3AUrN2Ffg9zdGN+jYB/nMlH2EOs+3ko7RidYO70u3V+BgNnjfSrq6\n"
"2edcFTVDKx418/WGOytibPUnnPHK7A3ZJ/WHldTVCqxXOXyWs6TN/3C/bFs9cpU7\n"
"WY8+e0zPfQA5jJ9Mb3bljAXLZAzqUsebMKGPufIwoXtJub+VNPxJwEPZrZfJspcJ\n"
"m3zJQlgDZBwzSYNRZFQZNdgSEBcoAfpiIeH58VX8aRHhmOTj4uo2AIo1XE2YkiBW\n"
"ZLm4Ssd/OpzACY0VL/PtNhIsVnr9mP0aL8DX4wIDAQABAoIBAFMAGHYj6PX8Pdlo\n"
"Yir94+HnK/bfhuYGby6jFIkj5ju2UEHWuzsxNOhPv9pqa2LxyK9QwzDyco9eqzE7\n"
"riAJXLwnbSRycbfZaQDweVXdb0i5Xjf+voxAnO4Gf+stgQ7Xs59MTXuMMHhhgnP8\n"
"M9CySE+/XTecGZk9DcV7PYCKC8Nc9KQ4raYydhCs7jZeeasBHFUzFy+Ep6YYuG01\n"
"BnXR7fmH/oer1+cj/Qj6nCub2TjtraWtbK+vNQ9Q5QFg3yNkZBliG2ftinjDjHVB\n"
"06SySe9NyySz4RmI5R7DF7yveypqoIduwANA2ua78ccSxtvqr0DBEaZN2tB8+E2+\n"
"ij8eXQkCgYEA/xUKn7t6I7Fm346BH+E9iJESKk2RlkLuQx+w9mlyuZjkLUGZgThS\n"
"HNAj0zDsSE70YVunl2Dz9fAHiMpjzqA+HTqvXOL/3sXupUyquoBgsKqe+sizyDDN\n"
"+QqZJSzaWp5t0aQIMTFqcd3Fgdj6PuONAYX2VlMrW9QMzH9U/A+ARVsCgYEAz7QG\n"
"Mwp2cXLT9BjvvhC/apRe37vPBC8FGaKMTPbTtiI+SeUhbt50d3vKncfUJziTVz4V\n"
"SYi5FGYXfz+musn9zN4pZ9MHFaclNiE438CTfSo36kUjWsquplFqkJ9BMMpQZpE4\n"
"nmx/w5GWcO0evjsMQ8PvRwjacY+lEFe536rA1hkCgYEA2Y3BpOBJ2J1aRvsMZ/s7\n"
"9vj7zDaiH8zv1zH1RQREg8TBf1O+r3YwmkBu/ZVxQG6OgWahdjuLdsuEMYekPjtv\n"
"HpDJtegpIIAh/Lt5tVz+mk67DtsUcn4lfe0rFoi9pqIOuckz73jc90aVSByduftr\n"
"bMwrgA6pIUWmsNc8zUoPNZcCgYEAsh/loSg745dihlhMYmfigPi1VJKwOxpH+XAZ\n"
"enfDoNNFMAI85eQJZd7YKPAS1YADfDJV9zY143SaehqQVmicLHHqeIvV64/orb9Y\n"
"EywIULNAOL0KUPa5SRFRnq21Lq6SvSOVtue9um7E4hu43dOt9P+32OeSzwktuhJB\n"
"6bt6nSECgYEAmNCbvkFcFJyOY6IdPMYB3C9znmnDGtAnbayHH8WoMOXHR+F/lHuq\n"
"7JezosE2MIquU21aPiHezg1X4bCKVQmWROKtoOFi+DyBYg0ph+6nRK2AirQEJRxP\n"
"8HWSjxqe4BL0F1ahp0H5uWlqTW1VBhUdZuLMhjT76a/TCZcqgKheQ+U=\n"
"-----END RSA PRIVATE KEY-----\n";

static bool WriteTextFile(
  const std::string& filename
  , const char* text
)
{
  std::ofstream f(filename, std::ios::binary);
  if (!f)
    return false;

  f.write(text, std::streamsize(strlen(text)));
  return bool(f);
}

static bool ParseArgs(
  int argc
  , char** argv
  , bool& ssl
  , std::string& certFile
  , std::string& keyFile
)
{
  ssl = false;
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

    if (a == "--help" || a == "help")
      return false;
  }

  return true;
}

static void PrintUsage(const char* exe)
{
  std::cout
    << "Usage:\n"
    << "  " << exe << " [--ssl [--cert file] [--key file]]\n"
    << "\n"
    << "Options:\n"
    << "  --ssl            Enable SSL for control server.\n"
    << "  --cert <file>    Path to a PEM certificate file.\n"
    << "  --key <file>     Path to a PEM private key file.\n"
    << "  --help           Show this help.\n"
    << "\n"
    << "Notes:\n"
    << "  - If --ssl is specified without --cert/--key, the demo certificate and key\n"
    << "    are written to the current directory as:\n"
    << "      dynamiccontrol_demo_cert.pem\n"
    << "      dynamiccontrol_demo_key.pem\n"
    << "  - For real usage, pass your own files via --cert and --key.\n"
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

  if (!ParseArgs(argc, argv, ssl, certFile, keyFile))
  {
    PrintUsage(argv[0]);
    return 1;
  }

  Logme::ChannelPtr t1 = Logme::Instance->CreateChannel(Logme::ID{"t1"});
  Logme::ChannelPtr t2 = Logme::Instance->CreateChannel(Logme::ID{"t2"});

  const int port = 7791;
  Logme::ControlConfig cfg{};
  cfg.Enable = true;
  cfg.Port = port;
  cfg.Interface = 0;

  X509* cert = nullptr;
  EVP_PKEY* key = nullptr;

  if (ssl)
  {
    if (certFile.empty() != keyFile.empty())
    {
      std::cerr << "ERROR: Both --cert and --key must be specified together." << std::endl;
      return 2;
    }

    if (certFile.empty() && keyFile.empty())
    {
      certFile = "dynamiccontrol_demo_cert.pem";
      keyFile = "dynamiccontrol_demo_key.pem";

      if (!WriteTextFile(certFile, DemoCertPem) || !WriteTextFile(keyFile, DemoKeyPem))
      {
        std::cerr << "ERROR: Unable to write demo SSL certificate/key files." << std::endl;
        return 2;
      }

      std::cout
        << "Demo SSL certificate/key files were generated:\n"
        << "  " << certFile << "\n"
        << "  " << keyFile << "\n"
        << "Use --cert/--key to provide your own files for real usage.\n"
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

  PrintInstructions(port);

  Worker w1("t1", "t1", "wrk1", "wrk1b");
  Worker w2("t2", "t2", "wrk2", "wrk2b");

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
