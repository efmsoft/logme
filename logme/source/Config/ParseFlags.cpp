#include <algorithm>

#include <Logme/Logme.h>

#include "Helper.h"

using namespace Logme;

#ifdef USE_JSONCPP
#include <json/json.h>

struct FLAG_CONFIG
{
  const char* Name;
  const NAMED_VALUE* Values;
  void (*SetFlag)(OutputFlags& f, int v);
};

static NAMED_VALUE TimeStampValues[] =
{
  {"", TIME_FORMAT_LOCAL},
  {"none", TIME_FORMAT_NONE},
  {"local", TIME_FORMAT_LOCAL},
  {"tz", TIME_FORMAT_TZ},
  {"utc", TIME_FORMAT_UTC},
  {nullptr, 0}
};

static NAMED_VALUE LocationValues[] =
{
  {"", DETALITY_NONE},
  {"none", DETALITY_NONE},
  {"short", DETALITY_SHORT},
  {"full", DETALITY_FULL},
  {nullptr, 0}
};

static NAMED_VALUE ConsoleValues[] =
{
  {"", STREAM_ALL2COUT},
  {"cout", STREAM_ALL2COUT},
  {"warncerr", STREAM_WARNCERR},
  {"errcerr", STREAM_ERRCERR},
  {"cerrcerr", STREAM_CERRCERR},
  {"cerr", STREAM_ALL2CERR},
  {nullptr, 0}
};

static FLAG_CONFIG FlagConfig[] =
{
  {"timestamp", TimeStampValues, [](OutputFlags& f, int v) {f.Timestamp = v; } },
  {"signature", nullptr, [](OutputFlags& f, int v) {f.Signature = v; } },
  {"location", LocationValues, [](OutputFlags& f, int v) {f.Location = v; } },
  {"method", nullptr, [](OutputFlags& f, int v) {f.Method = v; } },
  {"eol", nullptr, [](OutputFlags& f, int v) {f.Eol = v; } },
  {"errorprefix", nullptr, [](OutputFlags& f, int v) {f.ErrorPrefix = v; } },
  {"duration", nullptr, [](OutputFlags& f, int v) {f.Duration = v; } },
  {"threadid", nullptr, [](OutputFlags& f, int v) {f.ThreadID = v; } },
  {"processid", nullptr, [](OutputFlags& f, int v) {f.ProcessID = v; } },
  {"channel", nullptr, [](OutputFlags& f, int v) {f.Channel = v; } },
  {"highlight", nullptr, [](OutputFlags& f, int v) {f.Highlight = v; } },
  {"console", ConsoleValues, [](OutputFlags& f, int v) {f.Console = v; } },
  {"disablelink", nullptr, [](OutputFlags& f, int v) {f.DisableLink = v; } },
  {nullptr, nullptr, nullptr, }
};

bool ProcessFlag(
  OutputFlagsMap& m
  , const Json::String& flags_name
  , const Json::String& name
  , const Json::Value& o
  , OutputFlags& f
)
{
  std::string lcname(name);
  std::transform(lcname.begin(), lcname.end(), lcname.begin(), ::tolower);

  if (lcname == "inherit")
  {
    if (!o.isString())
    {
      LogmeE(CHINT, "flags.%s.%s is not a string value", flags_name.c_str(), name.c_str());
      return false;
    }

    auto parent = o.asString();
    auto it = m.find(parent);
    if (it == m.end())
    {
      LogmeE(CHINT, "flags.%s.%s is not a string value", flags_name.c_str(), name.c_str());
      return false;
    }

    f = m[parent];
    return true;
  }

  for (const FLAG_CONFIG* p = &FlagConfig[0]; p->Name; p++)
  {
    if (lcname != p->Name)
      continue;

    if (p->Values == nullptr)
    {
      if (!o.isBool())
      {
        LogmeE(CHINT, "flags.%s.%s is not a boolean value", flags_name.c_str(), name.c_str());
        return false;
      }

      p->SetFlag(f, o.asBool());
      return true;
    }

    if (!o.isString())
    {
      LogmeE(CHINT, "flags.%s.%s is not a string value", flags_name.c_str(), name.c_str());
      return false;
    }

    int v = 0;
    if (Name2Value(o.asString(), false, p->Values, v))
    {
      p->SetFlag(f, v);
      return true;
    }

    break;
  }

  LogmeW(CHINT, "flags.%s.%s contains unsupported value", flags_name.c_str(), name.c_str());
  return true;
}

static bool ParseFlags(
  OutputFlagsMap& m
  , const Json::String& name
  , const Json::Value& o
  , OutputFlags& f
)
{
  auto names = o.getMemberNames();

  for (Json::String n : names)
  {
    auto& v = o[n];
    if (!ProcessFlag(m, name, n, v, f))
      return false;
  }

  return true;
}

bool ParseFlags(const Json::Value& root, OutputFlagsMap& m)
{
  if (!root.isMember("flags"))
    return true;

  auto& a = root["flags"];

  if (!a.isObject())
  {
    LogmeE(CHINT, "\"flags\" is not an object");
    return false;
  }

  auto names = a.getMemberNames();
  for (auto& n : names)
  {
    auto& o = a[n];
    if (!o.isObject())
    {
      LogmeE(CHINT, "\"flags.%s\" is not an object", n.c_str());
      return false;
    }

    OutputFlags f;
    f.Value = 0;

    if (!ParseFlags(m, n, o, f))
      return false;

    m[n] = f;
  }

  return true;
}

#endif