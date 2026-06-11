#include <Logme/Logger.h>
#include <Logme/ThreadField.h>

using namespace Logme;

namespace
{
  thread_local ThreadFields CurrentThreadFields;

  bool IsValidName(const char* name)
  {
    return name != nullptr && *name != '\0';
  }
}

ThreadField::ThreadField()
{
}

ThreadField::ThreadField(const char* name, const char* value)
  : Name(name ? name : "")
  , Value(value ? value : "")
{
}

ThreadField::ThreadField(const std::string& name, const std::string& value)
  : Name(name)
  , Value(value)
{
}

ThreadFields::ThreadFields()
{
}

ThreadFieldArray::iterator ThreadFields::Find(const char* name)
{
  for (auto it = Fields.begin(); it != Fields.end(); ++it)
  {
    if (it->Name == name)
      return it;
  }

  return Fields.end();
}

ThreadFieldArray::const_iterator ThreadFields::Find(const char* name) const
{
  for (auto it = Fields.begin(); it != Fields.end(); ++it)
  {
    if (it->Name == name)
      return it;
  }

  return Fields.end();
}

void ThreadFields::Set(const char* name, const char* value)
{
  if (!IsValidName(name))
    return;

  auto it = Find(name);
  if (it == Fields.end())
  {
    Fields.emplace_back(name, value ? value : "");
  }
  else
  {
    it->Value = value ? value : "";
  }
}

void ThreadFields::Set(const std::string& name, const std::string& value)
{
  Set(name.c_str(), value.c_str());
}

bool ThreadFields::Get(const char* name, std::string& value) const
{
  if (!IsValidName(name))
    return false;

  auto it = Find(name);
  if (it == Fields.end())
    return false;

  value = it->Value;
  return true;
}

bool ThreadFields::Get(const std::string& name, std::string& value) const
{
  return Get(name.c_str(), value);
}

void ThreadFields::Remove(const char* name)
{
  if (!IsValidName(name))
    return;

  auto it = Find(name);
  if (it != Fields.end())
    Fields.erase(it);
}

void ThreadFields::Remove(const std::string& name)
{
  Remove(name.c_str());
}

void ThreadFields::Clear()
{
  Fields.clear();
}

bool ThreadFields::Empty() const
{
  return Fields.empty();
}

const ThreadFieldArray& ThreadFields::GetFields() const
{
  return Fields;
}

ThreadFieldsOverride::ThreadFieldsOverride(LoggerPtr logger, const ThreadFields& fields)
  : Logger(logger)
  , PrevFields(Logger->GetThreadFields())
{
  Logger->SetThreadFields(&fields);
}

ThreadFieldsOverride::~ThreadFieldsOverride()
{
  if (PrevFields.Empty())
    Logger->SetThreadFields(nullptr);
  else
    Logger->SetThreadFields(&PrevFields);
}

void Logger::SetThreadFields(const ThreadFields* fields)
{
  if (fields == nullptr)
  {
    CurrentThreadFields.Clear();
  }
  else
  {
    CurrentThreadFields = *fields;
  }
}

ThreadFields Logger::GetThreadFields()
{
  return CurrentThreadFields;
}

void Logger::SetThreadField(const char* name, const char* value)
{
  CurrentThreadFields.Set(name, value);
}

void Logger::SetThreadField(const std::string& name, const std::string& value)
{
  CurrentThreadFields.Set(name, value);
}

bool Logger::GetThreadField(const char* name, std::string& value)
{
  return CurrentThreadFields.Get(name, value);
}

bool Logger::GetThreadField(const std::string& name, std::string& value)
{
  return CurrentThreadFields.Get(name, value);
}

void Logger::RemoveThreadField(const char* name)
{
  CurrentThreadFields.Remove(name);
}

void Logger::RemoveThreadField(const std::string& name)
{
  CurrentThreadFields.Remove(name);
}

void Logger::ClearThreadFields()
{
  CurrentThreadFields.Clear();
}

ThreadFieldArray Logme::GetThreadFieldsSnapshot()
{
  return CurrentThreadFields.GetFields();
}
