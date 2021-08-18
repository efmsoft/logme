#include <Logme/Logger.h>
#include <Logme/Stream.h>

using namespace Logme;

Stream::Stream(LoggerPtr logger, const Context& context)
  : Destination(logger)
  , OutputContext(context)
{
}

Stream::Stream(const Stream& src)
  : Destination(src.Destination)
  , OutputContext(src.OutputContext)
{
}

Stream::~Stream()
{
  if (!str().empty())
    Destination->Log(OutputContext, str().c_str());
}
