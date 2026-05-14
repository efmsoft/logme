#include <string>

#include <Logme/Logme.h>

static bool LoadLogmeConfiguration(const std::string& path)
{
  std::string error;
  if (Logme::Instance->LoadConfigurationFile(path, std::string(), &error))
  {
    return true;
  }

  LogmeE("cannot load logme configuration '%s': %s", path.c_str(), error.c_str());
  return false;
}

static void ProcessRequest(int requestId)
{
  LogmeI("request %d accepted", requestId);
  LogmeW("request %d took longer than expected", requestId);
}

int main(int argc, char* argv[])
{
  const char* configPath = argc > 1
    ? argv[1]
    : "typical-usage.json";

  if (!LoadLogmeConfiguration(configPath))
  {
    return 1;
  }

  LogmeI("application started with configuration file '%s'", configPath);

  ProcessRequest(1001);
  LogmeE("example error record");

  return 0;
}
