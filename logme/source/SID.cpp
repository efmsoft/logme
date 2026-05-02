#include <cstring>

#include <Logme/Logme.h>
#include <Logme/SID.h>

using namespace Logme;

SID SID::Build(uint64_t name)
{
  return SID{name};
}

SID SID::Build(const char* name_upto8chars)
{
  SID sid{};

  if (name_upto8chars == nullptr)
    return sid;

  size_t len = std::strlen(name_upto8chars);
  if (len > sizeof(sid.Name) && Instance)
    LogmeW(CHINT, "subsystem name '%s' is longer than 8 characters and will be truncated", name_upto8chars);

  char* p = (char*)&sid.Name;
  for (size_t i = 0; i < sizeof(sid.Name) && *name_upto8chars; ++i)
    *p++ = *name_upto8chars++;

  return sid;
}

SID SID::Build(const std::string& name_upto8chars)
{
  return Build(name_upto8chars.c_str());
}
