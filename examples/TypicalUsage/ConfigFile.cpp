#include <string>

#include <Logme/Logme.h>

static const char* TYPICAL_USAGE_CONFIGURATION = R"json(
{
  "channels": [
    {
      "name": "",
      "level": "debug",
      "backends": [
        {
          "type": "ConsoleBackend",
          "async": false
        },
        {
          "type": "FileBackend",
          "append": true,
          "max-size": "1Mb",
          "file": "typical-usage-config.log"
        }
      ]
    }
  ]
}
)json";

static bool LoadLogmeConfiguration()
{
  std::string error;
  if (Logme::Instance->LoadConfiguration(TYPICAL_USAGE_CONFIGURATION, std::string(), &error))
  {
    return true;
  }

  LogmeE("cannot load logme configuration: %s", error.c_str());
  return false;
}

static void ProcessRequest(int requestId)
{
  LogmeI("request %d accepted", requestId);
  LogmeW("request %d took longer than expected", requestId);
}

int main()
{
  if (!LoadLogmeConfiguration())
  {
    return 1;
  }

  LogmeI("application started with embedded configuration");

  ProcessRequest(1001);
  LogmeE("example error record");

  return 0;
}
