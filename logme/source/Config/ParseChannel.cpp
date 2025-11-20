#include <Logme/Logme.h>
#include <Logme/SafeID.h>

#include "Helper.h"

using namespace Logme;

#ifdef USE_JSONCPP
#include <json/json.h>

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

bool Logme::LevelFromName(const std::string& n, int& v)
{
  if (!Name2Value(n, false, LevelValues, v))
    return false;

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
  if (!LevelFromName(o["level"].asString(), v))
  {
    LogmeE(CHINT, "\"channels[%i].level\" value is not supported", i);
    return false;
  }

  level = (Level)v;

  return true;
}

static bool GetEnabled(const Json::Value& o, int i, bool& enable)
{
  if (!o.isMember("enable"))
    return true;

  if (!o["enable"].isBool())
  {
    LogmeE(CHINT, "\"channels[%i].enable\" is not a bool", i);
    return false;
  }

  enable = o["enable"].asBool();
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

static bool GetLink(const Json::Value& o, int i, std::string& link)
{
  if (!o.isMember("link"))
    return false;

  if (!o["link"].isString())
  {
    LogmeE(CHINT, "\"channels[%i].link\" is not a string", i);
    return false;
  }

  link = o["link"].asString();

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

bool ParseChannels(
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

    if (!GetEnabled(o, i, c.Enabled))
      return false;

    if (!GetBackends(o, i, c.Backend))
      return false;

    std::string link;
    if (GetLink(o, i, link))
      c.Link = link;

    arr.push_back(c);
  }

  return true;
}

#endif

void Logger::ReplaceChannels(ChannelConfigArray& arr)
{
  std::vector<std::string> names;
  std::lock_guard guard(DataLock);

  DeleteAllChannels();

  for (auto& c : arr)
  {
    if (c.Object == nullptr)
      continue;

    std::string name = c.Object->GetName();

    if (name.empty())
      Default = c.Object;
    else
    {
      if (std::find(names.begin(), names.end(), name) != names.end())
        continue;

      Channels[name] = c.Object;
      names.push_back(name);
    }
  }

  CreateDefaultChannelLayout(false);
}

static ChannelPtr LookupChannel(ChannelConfigArray& arr, Logme::ID& ch)
{
  for (auto& c : arr)
  {
    if (c.Object == nullptr)
      continue;

    if (ch.Name == nullptr || *ch.Name == '\0')
      return c.Object;
    
    if (c.Name == ch.Name)
      return c.Object;
  }

  return ChannelPtr();
}

bool Logger::CreateChannels(ChannelConfigArray& arr)
{
  bool rc = true;
  for (auto& c : arr)
  {
    ID ch{ c.Name.c_str() };
    
    ChannelPtr channel;
    const char* name = ch.Name ? ch.Name : nullptr;
    channel = std::make_shared<Channel>(this, name, c.Flags, c.Filter);

    if (channel == nullptr)
    {
      rc = false;
      continue;
    }

    channel->SetEnabled(c.Enabled);

    for (auto& b : c.Backend)
    {
      BackendPtr backend = Backend::Create(b->Type.c_str(), channel);
      if (backend == nullptr)
      {
        rc = false;
        continue;
      }

      if (!backend->ApplyConfig(b))
      {
        rc = false;
        continue;
      }

      channel->AddBackend(backend);
    }

    c.Object = channel;
  }

  for (auto& c : arr)
  {
    if (c.Link.has_value())
    {
      SafeID ch1(c.Name.c_str());
      SafeID ch2(c.Link.value().c_str());

      auto p1 = LookupChannel(arr, ch1);
      auto p2 = LookupChannel(arr, ch2);

      if (p1 && p2)
        p1->AddLink(ch2);
    }
  }
  
  return rc;
}
