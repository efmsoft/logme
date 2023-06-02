#include <Logme/Logme.h>

#ifdef _WIN32
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

using namespace Logme;

#ifdef USE_JSONCPP
#include <json/json.h>

bool ParseControlConfig(const Json::Value& root, ControlConfig& cc)
{
  memset(&cc, 0, sizeof(cc));

  if (!root.isMember("control"))
    return true;

  auto& o = root["control"];

  if (!o.isObject())
  {
    LogmeE(CHINT, "\"control\" is not an object");
    return false;
  }

  if (o.isMember("enable"))
  {
    if (!o["enable"].isBool())
    {
      LogmeE(CHINT, "\"control.enable\" is not a boolean value");
      return false;
    }

    cc.Enable = o["enable"].asBool();
  }

  if (o.isMember("port"))
  {
    if (!o["port"].isInt())
    {
      LogmeE(CHINT, "\"control.port\" is not an integer value");
      return false;
    }

    cc.Port = o["port"].asInt();
  }

  if (o.isMember("interface"))
  {
    if (!o["interface"].isString())
    {
      LogmeE(CHINT, "\"control.interface\" is not a string value");
      return false;
    }

    auto iface = o["interface"].asString();
    if (iface == "loopback" || iface == "127.0.0.1")
    {
      cc.Interface = INADDR_LOOPBACK;
    }
    else if (iface == "" || iface == "any" || iface == "all")
    {
      cc.Interface = INADDR_ANY;
    }
    else
    {
      cc.Interface = inet_addr(iface.c_str());
      if (cc.Interface == INADDR_NONE)
        return false;
    }
  }

  if (cc.Enable && cc.Port == 0)
  {
    LogmeW(CHINT, "Disable control interface because of zero value of Port");
    cc.Enable = false;
  }

  return true;
}

#endif
