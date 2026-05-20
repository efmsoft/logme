#include <Logme/Logme.h>
#include <Logme/Backend/WindowsEventLogBackend.h>
#include <Logme/Channel.h>

#include <memory>

int main()
{
  Logme::ID id{"eventlog"};
  auto channel = Logme::Instance->CreateChannel(id);

  auto config = std::make_shared<Logme::WindowsEventLogBackendConfig>();
  config->Source = "LogmeExample";
  config->EventId = 1000;
  config->Category = 0;
  config->Async = true;

  auto backend = std::make_shared<Logme::WindowsEventLogBackend>(channel);
  backend->ApplyConfig(config);
  channel->AddBackend(backend);

  LogmeI(channel, "Windows Event Log information record");
  LogmeW(channel, "Windows Event Log warning record");
  LogmeE(channel, "Windows Event Log error record");

  backend->Flush();

  return 0;
}
