#include "Json.h"

#include <cstdio>

using namespace Logme;

JsonValue::JsonValue()
  : T(Type::Null)
  , B(false)
  , N(0)
{
}

JsonValue::JsonValue(bool value)
  : T(Type::Bool)
  , B(value)
  , N(0)
{
}

JsonValue::JsonValue(int64_t value)
  : T(Type::Number)
  , B(false)
  , N(value)
{
}

JsonValue::JsonValue(uint64_t value)
  : T(Type::Number)
  , B(false)
  , N((int64_t)value)
{
}

JsonValue::JsonValue(const char* value)
  : T(Type::String)
  , B(false)
  , N(0)
  , S(value ? value : "")
{
}

JsonValue::JsonValue(const std::string& value)
  : T(Type::String)
  , B(false)
  , N(0)
  , S(value)
{
}

JsonValue JsonValue::Object()
{
  JsonValue v;
  v.T = Type::Object;
  return v;
}

JsonValue JsonValue::Array()
{
  JsonValue v;
  v.T = Type::Array;
  return v;
}

JsonValue::Type JsonValue::GetType() const
{
  return T;
}

JsonValue& JsonValue::Set(const std::string& key, const JsonValue& value)
{
  if (T != Type::Object)
  {
    T = Type::Object;
    O.clear();
    A.clear();
    S.clear();
    N = 0;
    B = false;
  }

  O[key] = value;
  return *this;
}

JsonValue& JsonValue::Push(const JsonValue& value)
{
  if (T != Type::Array)
  {
    T = Type::Array;
    A.clear();
    O.clear();
    S.clear();
    N = 0;
    B = false;
  }

  A.push_back(value);
  return *this;
}

std::string JsonValue::ToString() const
{
  std::string out;
  Append(out, *this);
  return out;
}

void JsonValue::AppendEscaped(std::string& out, const std::string& s)
{
  out.push_back('"');
  for (unsigned char c : s)
  {
    switch (c)
    {
    case '\\': out += "\\\\"; break;
    case '"': out += "\\\""; break;
    case '\b': out += "\\b"; break;
    case '\f': out += "\\f"; break;
    case '\n': out += "\\n"; break;
    case '\r': out += "\\r"; break;
    case '\t': out += "\\t"; break;
    default:
      if (c < 0x20)
      {
        char buf[8];
        snprintf(buf, sizeof(buf), "\\u%04x", (unsigned)c);
        out += buf;
      }
      else
      {
        out.push_back((char)c);
      }
      break;
    }
  }
  out.push_back('"');
}

void JsonValue::Append(std::string& out, const JsonValue& v)
{
  switch (v.T)
  {
  case Type::Null:
    out += "null";
    break;
  case Type::Bool:
    out += (v.B ? "true" : "false");
    break;
  case Type::Number:
  {
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", (long long)v.N);
    out += buf;
    break;
  }
  case Type::String:
    AppendEscaped(out, v.S);
    break;
  case Type::Object:
  {
    out.push_back('{');
    bool first = true;
    for (auto& kv : v.O)
    {
      if (!first)
        out.push_back(',');
      first = false;
      AppendEscaped(out, kv.first);
      out.push_back(':');
      Append(out, kv.second);
    }
    out.push_back('}');
    break;
  }
  case Type::Array:
  {
    out.push_back('[');
    bool first = true;
    for (auto& it : v.A)
    {
      if (!first)
        out.push_back(',');
      first = false;
      Append(out, it);
    }
    out.push_back(']');
    break;
  }
  }
}
