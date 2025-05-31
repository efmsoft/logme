#include <sstream>

#include <Logme/Json/Loader.h>
#include <Logme/Utils.h>

using namespace Logme::JSON;

static const Json::Value* FindByPath(
  const Json::Value& root
  , const std::string& path
)
{
  const Json::Value* current = &root;
  
  std::string part;
  std::istringstream ss(path);

  while (std::getline(ss, part, '/')) 
  {
    if (!current->isObject() || !current->isMember(part))
      return nullptr;
    
    current = &(*current)[part];
  }

  return current;
}

bool Logme::JSON::Load(
  const Json::Value& source
  , const FieldArray& fields
  , std::string& error
)
{
  for (const auto& entry : fields) 
  {
    const Json::Value* value = FindByPath(source, entry.Path);

    if (!value || value->isNull()) 
    {
      if (entry.Required) 
      {
        error = "Missing required field: " + entry.Path;
        return false;
      }
      else
        value = &entry.DefaultValue;
    }

    switch (entry.Type) 
    {
    case TYPE_INT:
      if (!value->isInt()) 
      {
        error = "Expected int at " + entry.Path;
        return false;
      }

      *static_cast<int*>(entry.Target) = value->asInt();
      break;

    case TYPE_DOUBLE:
      if (!value->isDouble() && !value->isInt()) 
      {
        error = "Expected double at " + entry.Path;
        return false;
      }

      *static_cast<double*>(entry.Target) = value->asDouble();
      break;

    case TYPE_STRING:
      if (!value->isString()) 
      {
        error = "Expected string at " + entry.Path;
        return false;
      }

      *static_cast<std::string*>(entry.Target) = value->asString();
      break;

    case TYPE_BOOL:
      if (!value->isBool()) 
      {
        error = "Expected bool at " + entry.Path;
        return false;
      }

      *static_cast<bool*>(entry.Target) = value->asBool();
      break;
    }
  }

  return true;
}

bool Logme::JSON::Name2Value(const NamedValues& table, const std::string& name, uint64_t& value)
{
  std::string s(name);
  ToLowerAsciiInplace(s);

  for (auto& i : table)
  {
    std::string c(i.first);
    ToLowerAsciiInplace(s);

    if (s == c)
    {
      value = i.second;
      return true;
    }
  }
  return false;
}
