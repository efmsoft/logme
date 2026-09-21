#include <Logme/Logger.h>
#include <Logme/ThreadCondition.h>

using namespace Logme;

ThreadCondition::ThreadCondition(LoggerPtr logger, bool condition)
  : Logger(logger)
  , HadPrev(Logger->IsLogConditionDefinedForCurrentThread())
  , PrevCondition(Logger->GetThreadLogCondition())
{
  Logger->SetThreadLogCondition(&condition);
}

ThreadCondition::~ThreadCondition()
{
  if (HadPrev)
    Logger->SetThreadLogCondition(&PrevCondition);
  else
    Logger->SetThreadLogCondition(nullptr);
}
