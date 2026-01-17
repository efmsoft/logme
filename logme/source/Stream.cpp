#include <Logme/Logger.h>
#include <Logme/Stream.h>

using namespace Logme;

Stream::Stream(LoggerPtr logger, const Context& context, OverridePtr ovr)
  : Destination(logger)
  , OutputContext(context)
  , Ovr(ovr)
{
}

Stream::Stream(const Stream& src)
  : std::stringstream()
  , Destination(src.Destination)
  , OutputContext(src.OutputContext)
  , Ovr(src.Ovr)
{
}

Stream::~Stream()
{
  const std::string& data = str();

  if (data.length())
  {
    if (OutputContext.Channel)
    {
      if (OutputContext.Ovr)
      {
        Destination->Log(
          OutputContext
          , *OutputContext.Channel
          , *OutputContext.Ovr
          , "%s"
          , data.c_str()
        );
      }
      else
      {
        Destination->Log(
          OutputContext
          , *OutputContext.Channel
          , "%s"
          , data.c_str()
        );
      }
    }
    else
    {
      if (OutputContext.Ovr)
      {
        Destination->Log(OutputContext, *OutputContext.Ovr, "%s", data.c_str());
      }
      else
        Destination->Log(OutputContext, "%s", data.c_str());
    }
  }
}
