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
  const std::string& data = str();

  if (data.length())
    Destination->Log(OutputContext, "%s", data.c_str());
}
