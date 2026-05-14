#include <memory>

#include <Logme/Backend/ConsoleBackend.h>
#include <Logme/Backend/FileBackend.h>
#include <Logme/Logme.h>

static Logme::ChannelPtr CreateApplicationChannel()
{
  Logme::ID id{"app"};

  auto ch = Logme::Instance->CreateChannel(id, Logme::OutputFlags(), Logme::LEVEL_DEBUG);

  auto console = std::make_shared<Logme::ConsoleBackend>(ch);
  console->SetAsync(true);
  ch->AddBackend(console);

  auto file = std::make_shared<Logme::FileBackend>(ch);
  file->SetAppend(true);
  file->SetMaxSize(1024ULL * 1024);
  
  if (file->CreateLog("typical-usage.log") == false)
  {
    LogmeE("cannot create log file 'typical-usage.log'");
    exit(1);
  }

  ch->AddBackend(file);

  return ch;
}

static void ProcessRequest(
  const Logme::ChannelPtr& ch
  , int requestId
)
{
  Logme::SID network = Logme::SID::Build("network");
  Logme::SID storage = Logme::SID::Build("storage");

  LogmeI(ch, network, "request %d accepted", requestId);
  LogmeD(ch, storage, "request %d loaded cached profile", requestId);
  LogmeW(ch, network, "request %d took longer than expected", requestId);
}

int main()
{
  auto app = CreateApplicationChannel();

  LogmeI(app, "application started with code configuration");

  ProcessRequest(app, 1001);
  LogmeE(app, "example error record");

  return 0;
}
