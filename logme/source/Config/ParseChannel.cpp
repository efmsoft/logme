#include <Logme/Logme.h>

#include "Helper.h"

#ifdef USE_JSONCPP
#include <json/json.h>

using namespace Logme;

static NAMED_VALUE LevelValues[] =
{
  {"", DEFAULT_LEVEL},
  {"info", LEVEL_INFO},
  {"information", LEVEL_INFO},
  {"debug", LEVEL_DEBUG},
  {"warn", LEVEL_WARN},
  {"warning", LEVEL_WARN},
  {"err", LEVEL_ERROR},
  {"error", LEVEL_ERROR},
  {"crit", LEVEL_CRITICAL},
  {"ctitical", LEVEL_CRITICAL},
  {nullptr, 0}
};

static bool GetChannelName(const Json::Value& o, int i, std::string& name)
{
  if (!o.isMember("name"))
  {
    LogmeE(CHINT, "\"channels[%i]\" does not contain \"name\" member", i);
    return false;
  }

  if (!o["name"].isString())
  {
    LogmeE(CHINT, "\"channels[%i].name\" is not a string", i);
    return false;
  }

  name = o["name"].asString();

  return true;
}

static bool GetFilterLevel(const Json::Value& o, int i, Level& level)
{
  if (!o.isMember("level"))
    return true;

  if (!o["level"].isString())
  {
    LogmeE(CHINT, "\"channels[%i].level\" is not a string", i);
    return false;
  }

  int v = 0;
  if (!Name2Value(o["level"].asString(), false, LevelValues, v))
  {
    LogmeE(CHINT, "\"channels[%i].level\" value is not supported", i);
    return false;
  }

  level = (Level)v;

  return true;
}

static bool GetChannelFlags(const Json::Value& o, int i, OutputFlagsMap& m, OutputFlags& f)
{
  if (!o.isMember("flags"))
    return true;
  
  if (!o["flags"].isString())
  {
    LogmeE(CHINT, "\"channels[%i].flags\" is not a string", i);
    return false;
  }

  Json::String flags_name = o["flags"].asString();
  auto it = m.find(flags_name);

  if (it == m.end())
  {
    LogmeE(CHINT, "\"channels[%i].flags\" is not defined", i);
    return false;
  }

  f = m[flags_name];

  return true;
}

static bool PlatformOK(const Json::Value& o, int i, int j, bool& ok)
{
  ok = true;
  if (!o.isMember("platform"))
    return true;

  if (!o["platform"].isString())
  {
    if (j == -1)
    {
      LogmeE(CHINT, "\"channels[%i].platform\" is not a string", i);
    }
    else
    {
      LogmeE(CHINT, "\"channels[%i].backends[%i].platform\" is not a string", i, j);
    }
    return false;
  }

  if (!CheckPlatform(o["platform"].asString(), ok))
    return false;

  return true;
}

static bool BuildOK(const Json::Value& o, int i, int j, bool& ok)
{
  ok = true;
  if (!o.isMember("build"))
    return true;

  if (!o["build"].isString())
  {
    if (j == -1)
    {
      LogmeE(CHINT, "\"channels[%i].build\" is not a string", i);
    }
    else
    {
      LogmeE(CHINT, "\"channels[%i].backends[%i].build\" is not a string", i, j);
    }
    return false;
  }

  if (!CheckBuild(o["build"].asString(), ok))
    return false;

  return true;
}

static bool ParseBackend(const Json::Value& o, int i, int j, BackendConfigArray& arr)
{
  if (!o.isMember("type"))
  {
    LogmeE(CHINT, "\"channels[%i].backends[%i].type\" is not defined", i, j);
    return false;
  }

  if (!o["type"].isString())
  {
    LogmeE(CHINT, "\"channels[%i].backends[%i].type\" is not a string", i, j);
    return false;
  }

  std::string type = o["type"].asString();
  BackendPtr backend = Backend::Create(type.c_str(), ChannelPtr());
  
  if (backend == nullptr)
  {
    LogmeE(CHINT, "\"channels[%i].backends[%i].type\" is not supported", i, j);
    return false;
  }

  auto c = backend->CreateConfig();
  if (c == nullptr)
    return false;

  c->Type = type;
  if (!c->Parse(&o))
    return false;

  arr.push_back(c);

  return true;
}

static bool GetBackends(const Json::Value& o, int i, BackendConfigArray& arr)
{
  if (!o.isMember("backends"))
    return true;

  auto& b = o["backends"];

  if (!b.isArray())
  {
    LogmeE(CHINT, "\"channels[%i].backends\" is not an array of objects", i);
    return false;
  }

  bool ok = true;
  for (Json::Value::ArrayIndex j = 0; j < b.size(); ++j)
  {
    auto& o = b[j];

    if (!o.isObject())
    {
      LogmeE(CHINT, "\"channels[%i].backends[%i]\" is not an objects", i, j);
      return false;
    }

    if (!PlatformOK(o, i, j, ok))
      return false;
    else if (!ok)
      continue;

    if (!BuildOK(o, i, j, ok))
      return false;
    else if (!ok)
      continue;

    if (!ParseBackend(o, i, j, arr))
      return false;
  }

  return true;
}

bool PraseChannels(
  const Json::Value& root
  , OutputFlagsMap& m
  , ChannelConfigArray& arr
)
{
  if (!root.isMember("channels"))
    return true;

  auto& c = root["channels"];

  if (!c.isArray())
  {
    LogmeE(CHINT, "\"channels\" is not an array of objects");
    return false;
  }

  bool ok = true;
  for (Json::Value::ArrayIndex i = 0; i < c.size(); ++i)
  {
    auto& o = c[i];

    if (!o.isObject())
    {
      LogmeE(CHINT, "\"channels[%i]\" is not an objects", i);
      return false;
    }

    if (!PlatformOK(o, i, -1, ok))
      return false;
    else if (!ok)
      continue;

    if (!BuildOK(o, i, -1, ok))
      return false;
    else if (!ok)
      continue;

    ChannelConfig c;

    if (!GetChannelName(o, i, c.Name))
      return false;

    if (!GetChannelFlags(o, i, m, c.Flags))
      return false;

    if (!GetFilterLevel(o, i, c.Filter))
      return false;

    if (!GetBackends(o, i, c.Backend))
      return false;

    arr.push_back(c);
  }

  return true;
}

#endif

bool Logger::CreateChannels(ChannelConfigArray& arr)
{
  for (auto& c : arr)
  {
    ID ch{ c.Name.c_str() };
    
    ChannelPtr channel;
    if (*ch.Name != '\0')
      channel = CreateChannel(ch, c.Flags, c.Filter);
    else
    {
      channel = std::make_shared<Channel>(this, nullptr, c.Flags, c.Filter);
      Default = channel;
    }

    if (channel == nullptr)
      return false;

    for (auto& b : c.Backend)
    {
      BackendPtr backend = Backend::Create(b->Type.c_str(), channel);
      if (backend == nullptr)
        return false;

      if (!backend->ApplyConfig(b))
        return false;

      channel->AddBackend(backend);
    }
  }

  return true;
}
