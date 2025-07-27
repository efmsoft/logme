#pragma once

#include <string>
#include <vector>
#include <utility>

#ifdef USE_JSONCPP
#include <json/json.h>

#include <Logme/Types.h>

namespace Logme
{
  namespace JSON
  {
    enum ValueType
    {
      TYPE_INT,
      TYPE_DOUBLE,
      TYPE_STRING,
      TYPE_BOOL
    };

    struct FieldEntry
    {
      std::string Path;
      ValueType Type;
      void* Target;
      bool Required;
      Json::Value DefaultValue;
    };

    typedef std::vector<FieldEntry> FieldArray;

    LOGMELNK bool Load(
      const Json::Value& source
      , const FieldArray& fields
      , std::string& error
    );

    typedef std::vector<std::pair<const char*, uint64_t>> NamedValues;
    LOGMELNK bool Name2Value(const NamedValues& table, const std::string& name, uint64_t& value);

    template<typename T>
    bool ResolveName(const std::string& str, const NamedValues& table, T& value)
    {
      uint64_t v{};
      if (Name2Value(table, str, v) == false)
        return false;

      value = (T)v;
      return true;
    }
  }
}
#endif