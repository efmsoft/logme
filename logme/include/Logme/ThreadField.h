#pragma once

#include <memory>
#include <string>
#include <vector>

#include <Logme/Types.h>

namespace Logme
{
  class Logger;
  typedef std::shared_ptr<Logger> LoggerPtr;

  struct ThreadField
  {
    std::string Name;
    std::string Value;

    LOGMELNK ThreadField();
    LOGMELNK ThreadField(const char* name, const char* value);
    LOGMELNK ThreadField(const std::string& name, const std::string& value);
  };

  typedef std::vector<ThreadField> ThreadFieldArray;

  class ThreadFields
  {
    ThreadFieldArray Fields;

    ThreadFieldArray::iterator Find(const char* name);
    ThreadFieldArray::const_iterator Find(const char* name) const;

  public:
    LOGMELNK ThreadFields();

    LOGMELNK void Set(const char* name, const char* value);
    LOGMELNK void Set(const std::string& name, const std::string& value);
    LOGMELNK bool Get(const char* name, std::string& value) const;
    LOGMELNK bool Get(const std::string& name, std::string& value) const;
    LOGMELNK void Remove(const char* name);
    LOGMELNK void Remove(const std::string& name);
    LOGMELNK void Clear();
    LOGMELNK bool Empty() const;
    LOGMELNK const ThreadFieldArray& GetFields() const;
  };

  class ThreadFieldsOverride
  {
    LoggerPtr Logger;
    ThreadFields PrevFields;

  public:
    LOGMELNK ThreadFieldsOverride(LoggerPtr logger, const ThreadFields& fields);
    LOGMELNK ~ThreadFieldsOverride();
  };

  LOGMELNK ThreadFieldArray GetThreadFieldsSnapshot();
}
