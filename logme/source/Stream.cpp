#include <Logme/Logger.h>
#include <Logme/Stream.h>

using namespace Logme;

static void InitStreamContext(Context& dst, const Context& src)
{
  dst.ChannelStg = src.ChannelStg;

  if (src.Channel == &src.ChannelStg)
    dst.Channel = &dst.ChannelStg;
  else
    dst.Channel = src.Channel;

  dst.Subsystem = src.Subsystem;
  dst.Method = src.Method;
  dst.File = src.File;
  dst.Line = src.Line;

  dst.ChRef = src.ChRef;
  dst.Ch = src.Ch;

  dst.AppendProc = src.AppendProc;
  dst.AppendContext = src.AppendContext;

  dst.Ovr = src.Ovr;
  dst.Output = src.Output;

  dst.Signature = src.Signature;

  dst.Applied.ProcPrint = src.Applied.ProcPrint;
  dst.Applied.ProcPrintIn = src.Applied.ProcPrintIn;
}

Stream::Stream(LoggerPtr logger, const Context& context, OverridePtr ovr)
  : Destination(logger)
  , OutputContext(context.Cache, context.ErrorLevel, context.Channel, &context.Subsystem)
  , Ovr(ovr)
{
  InitStreamContext(OutputContext, context);
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
