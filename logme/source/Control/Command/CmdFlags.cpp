#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>

#include <Logme/Channel.h>
#include <Logme/Logger.h>
#include <Logme/OutputFlags.h>

#include "../CommandRegistrar.h"

using namespace Logme;

COMMAND_DESCRIPTOR2("flags", Logger::CommandFlags);

struct NamedValue
{
  const char* Name;
  int Value;
};

static NamedValue TimeStampValues[] =
{
  {"", TIME_FORMAT_LOCAL},
  {"none", TIME_FORMAT_NONE},
  {"local", TIME_FORMAT_LOCAL},
  {"tz", TIME_FORMAT_TZ},
  {"utc", TIME_FORMAT_UTC},
  {nullptr, 0}
};

static NamedValue LocationValues[] =
{
  {"", DETALITY_NONE},
  {"none", DETALITY_NONE},
  {"short", DETALITY_SHORT},
  {"full", DETALITY_FULL},
  {nullptr, 0}
};

static NamedValue ConsoleValues[] =
{
  {"", STREAM_ALL2COUT},
  {"cout", STREAM_ALL2COUT},
  {"warncerr", STREAM_WARNCERR},
  {"errcerr", STREAM_ERRCERR},
  {"cerrcerr", STREAM_CERRCERR},
  {"cerr", STREAM_ALL2CERR},
  {nullptr, 0}
};

static std::string ToLower(std::string s)
{
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
  return s;
}

static bool ParseBool(const std::string& s, bool& out)
{
  std::string v = ToLower(s);
  if (v.empty() || v == "1" || v == "true" || v == "on" || v == "yes")
  {
    out = true;
    return true;
  }

  if (v == "0" || v == "false" || v == "off" || v == "no")
  {
    out = false;
    return true;
  }

  return false;
}

static bool ParseInt(const std::string& s, int& out)
{
  if (s.empty())
    return false;

  char* end = nullptr;
  long v = std::strtol(s.c_str(), &end, 10);
  if (!end || *end != 0)
    return false;

  out = (int)v;
  return true;
}

static bool FindNamed(const NamedValue* values, const std::string& s, int& out)
{
  std::string v = ToLower(s);
  for (size_t i = 0; values[i].Name != nullptr; ++i)
  {
    if (v == values[i].Name)
    {
      out = values[i].Value;
      return true;
    }
  }
  return false;
}

static bool DefaultEnumValue1(const NamedValue* values, int& out)
{
  for (size_t i = 0; values[i].Name != nullptr; ++i)
  {
    if (values[i].Value == 1)
    {
      out = values[i].Value;
      return true;
    }
  }
  return false;
}

static bool GetChannelFromArgs(
  Logme::StringArray& arr
  , size_t& index
  , ChannelPtr& ch
  , std::string& error
)
{
  if (index + 1 < arr.size() && arr[index] == "--channel")
  {
    const std::string& name = arr[index + 1];
    if (name.empty())
    {
      error = "invalid channel name";
      return false;
    }

    ch = Logme::Instance->GetExistingChannel(ID{name.c_str()});
    if (ch == nullptr)
    {
      error = "no such channel: " + name;
      return false;
    }

    index += 2;
    return true;
  }

  ch = Logme::Instance->GetExistingChannel(::CH);
  if (ch == nullptr)
  {
    error = "no default channel";
    return false;
  }

  return true;
}

static std::string FormatFlags(const OutputFlags& f)
{
  char buf[32];
  snprintf(buf, sizeof(buf), "0x%08X", (unsigned)f.Value);
  return std::string("Flags: ") + buf + " " + f.ToString(" ", true);
}

bool Logger::CommandFlags(Logme::StringArray& arr, std::string& response)
{
  size_t index = 1;
  ChannelPtr ch;
  std::string error;

  std::lock_guard guard(Instance->DataLock);

  if (!GetChannelFromArgs(arr, index, ch, error))
  {
    response = "error: " + error;
    return true;
  }

  if (index >= arr.size())
  {
    response = FormatFlags(ch->GetFlags());
    return true;
  }

  OutputFlags f = ch->GetFlags();

  for (; index < arr.size(); ++index)
  {
    std::string token = arr[index];
    if (token.empty())
      continue;

    std::string name;
    std::string value;

    size_t eq = token.find('=');
    if (eq == std::string::npos)
    {
      name = token;
      value.clear();
    }
    else
    {
      name = token.substr(0, eq);
      value = token.substr(eq + 1);
    }

    name = ToLower(name);

    auto setBool = [&](auto&& setter)
    {
      bool b = true;
      if (!value.empty())
      {
        if (!ParseBool(value, b))
        {
          response = "error: invalid value for " + name + ": " + value;
          return false;
        }
      }
      setter(b);
      return true;
    };

    if (name == "timestamp")
    {
      int v = 0;
      if (value.empty())
      {
        if (!DefaultEnumValue1(TimeStampValues, v))
          v = TIME_FORMAT_LOCAL;
      }
      else if (!FindNamed(TimeStampValues, value, v) && !ParseInt(value, v))
      {
        response = "error: invalid value for timestamp: " + value;
        return true;
      }
      f.Timestamp = v;
      continue;
    }

    if (name == "location")
    {
      int v = 0;
      if (value.empty())
      {
        if (!DefaultEnumValue1(LocationValues, v))
          v = DETALITY_SHORT;
      }
      else if (!FindNamed(LocationValues, value, v) && !ParseInt(value, v))
      {
        response = "error: invalid value for location: " + value;
        return true;
      }
      f.Location = v;
      continue;
    }

    if (name == "console")
    {
      int v = 0;
      if (value.empty())
      {
        if (!DefaultEnumValue1(ConsoleValues, v))
          v = STREAM_ALL2COUT;
      }
      else if (!FindNamed(ConsoleValues, value, v) && !ParseInt(value, v))
      {
        response = "error: invalid value for console: " + value;
        return true;
      }
      f.Console = v;
      continue;
    }

    if (name == "signature")
    {
      if (!setBool([&](bool b)
      {
        f.Signature = b ? 1u : 0u;
      }))
        return true;
      continue;
    }

    if (name == "method")
    {
      if (!setBool([&](bool b)
      {
        f.Method = b ? 1u : 0u;
      }))
        return true;
      continue;
    }

    if (name == "eol")
    {
      if (!setBool([&](bool b)
      {
        f.Eol = b ? 1u : 0u;
      }))
        return true;
      continue;
    }

    if (name == "errorprefix")
    {
      if (!setBool([&](bool b)
      {
        f.ErrorPrefix = b ? 1u : 0u;
      }))
        return true;
      continue;
    }

    if (name == "duration")
    {
      if (!setBool([&](bool b)
      {
        f.Duration = b ? 1u : 0u;
      }))
        return true;
      continue;
    }

    if (name == "threadid")
    {
      if (!setBool([&](bool b)
      {
        f.ThreadID = b ? 1u : 0u;
      }))
        return true;
      continue;
    }

    if (name == "processid")
    {
      if (!setBool([&](bool b)
      {
        f.ProcessID = b ? 1u : 0u;
      }))
        return true;
      continue;
    }

    if (name == "channel")
    {
      if (!setBool([&](bool b)
      {
        f.Channel = b ? 1u : 0u;
      }))
        return true;
      continue;
    }

    if (name == "highlight")
    {
      if (!setBool([&](bool b)
      {
        f.Highlight = b ? 1u : 0u;
      }))
        return true;
      continue;
    }

    if (name == "disablelink")
    {
      if (!setBool([&](bool b)
      {
        f.DisableLink = b ? 1u : 0u;
      }))
        return true;
      continue;
    }

    if (name == "transition")
    {
      if (!setBool([&](bool b)
      {
        f.ThreadTransition = b ? 1u : 0u;
      }))
        return true;
      continue;
    }

    response = "error: unknown flag: " + name;
    return true;
  }

  ch->SetFlags(f);
  response = FormatFlags(ch->GetFlags()) + "\nok";
  return true;
}
