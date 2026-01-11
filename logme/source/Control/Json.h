#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Logme
{
  class JsonValue
  {
  public:
    enum class Type
    {
      Null,
      Bool,
      Number,
      String,
      Object,
      Array
    };

    JsonValue();
    explicit JsonValue(bool value);
    explicit JsonValue(int64_t value);
    explicit JsonValue(uint64_t value);
    explicit JsonValue(const char* value);
    explicit JsonValue(const std::string& value);

    static JsonValue Object();
    static JsonValue Array();

    Type GetType() const;

    JsonValue& Set(const std::string& key, const JsonValue& value);
    JsonValue& Push(const JsonValue& value);

    std::string ToString() const;

  private:
    static void AppendEscaped(std::string& out, const std::string& s);
    static void Append(std::string& out, const JsonValue& v);

    Type T;
    bool B;
    int64_t N;
    std::string S;
    std::map<std::string, JsonValue> O;
    std::vector<JsonValue> A;
  };
}
