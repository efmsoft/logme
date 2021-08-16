#include <Logme/Template.h>
#include <Logme/Types.h>

#include <map>
#include <mutex>
#include <string>

using namespace Logme;

typedef std::map<std::string, std::string> Environment;

namespace Logme
{
  Environment Env;
  std::mutex EnvLock;
}

void Logme::EnvSetVar(const char* name, const char* value)
{
  if (!name)
    return;

  Guard guard(EnvLock);

  if (!value || !*value)
  {
    Environment::iterator it = Env.find(name);
    if (it != Env.end())
      Env.erase(it);
  }
  else
    Env[name] = value;
}

std::string Logme::EnvGetVar(const char* name)
{
  if (!name)
    return "";

  Guard guard(EnvLock);

  std::string value;
  Environment::iterator it = Env.find(name);
  if (it != Env.end())
    value = it->second;

  return value;
}
