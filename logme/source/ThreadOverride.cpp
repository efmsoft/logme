#include <Logme/Logger.h>
#include <Logme/ThreadOverride.h>

using namespace Logme;

ThreadOverride::ThreadOverride(LoggerPtr logger, const Override& ovr)
  : Logger(logger)
  , PrevOverride(Logger->GetThreadOverride())
{
  Logger->SetThreadOverride(&ovr);
}

ThreadOverride::~ThreadOverride()
{
  if (PrevOverride.Add.Value == 0 && PrevOverride.Remove.Value == 0)
    Logger->SetThreadOverride(nullptr);
  else
    Logger->SetThreadOverride(&PrevOverride);
}
