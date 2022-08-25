#include <Logme/Logger.h>
#include <Logme/ThreadChannel.h>

using namespace Logme;

ThreadChannel::ThreadChannel(LoggerPtr logger, const ID& ch)
  : Logger(logger)
  , PrevChannel(Logger->GetDefaultChannel())
{
  Logger->SetThreadChannel(&ch);
}

ThreadChannel::~ThreadChannel()
{
  if (PrevChannel.Name == nullptr || *PrevChannel.Name == '\0')
    Logger->SetThreadChannel(nullptr);
  else
    Logger->SetThreadChannel(&PrevChannel);
}
