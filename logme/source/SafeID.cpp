#include <Logme/SafeID.h>

using namespace Logme;

SafeID::SafeID()
  : ID{}
{
}

SafeID::SafeID(const ID& id)
  : ID{}
{
  (*this) = id.Name;
}

SafeID::SafeID(const SafeID& id)
{
  (*this) = id.Name;
}

SafeID::SafeID(const char* name)
{
  (*this) = name;
}

SafeID& SafeID::operator=(const ID& id)
{
  return (*this) = id.Name;
}

SafeID& SafeID::operator=(const SafeID& id)
{
  return (*this) = id.Name;
}

SafeID& SafeID::operator=(const char* name)
{
  if (name)
    Storage = name;
  else
    Storage.clear();

  Name = Storage.c_str();
  return *this;
}