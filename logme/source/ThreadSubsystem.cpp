#include <Logme/Logger.h>
#include <Logme/ThreadSubsystem.h>

using namespace Logme;

ThreadSubsystem::ThreadSubsystem(LoggerPtr logger, const SID& sid)
  : Logger(logger)
  , PrevSubsystem(Logger->GetDefaultSubsystem())
{
  Logger->SetThreadSubsystem(&sid);
}

ThreadSubsystem::~ThreadSubsystem()
{
  if (PrevSubsystem.Name == 0)
    Logger->SetThreadSubsystem(nullptr);
  else
    Logger->SetThreadSubsystem(&PrevSubsystem);
}
