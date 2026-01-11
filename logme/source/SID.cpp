#include <Logme/SID.h>

using namespace Logme;

SID SID::Build(uint64_t name)
{
  return SID{name};
}

SID SID::Build(const char* name_upto8chars)
{
  SID sid{};

  char* p = (char*)&sid.Name;
  for (int i = 0; i < sizeof(sid.Name) && *name_upto8chars; ++i)
    *p++ = *name_upto8chars++;

  return sid;
}

SID SID::Build(const std::string& name_upto8chars)
{
  return Build(name_upto8chars.c_str());
}
