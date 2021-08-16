#include <Logme/Logger.h>
#include <Logme/Convert.h>

#if !defined(__GNUC__) && !defined(__GNUG__)
#include <codecvt>
#endif

using namespace Logme;

Stream::Stream(LoggerPtr logger, const Context& context)
  : Destination(logger)
  , OutputContext(context)
{
}

Stream::~Stream()
{
  Destination->Log(OutputContext, str().c_str());
}

WStream::WStream(LoggerPtr logger, const Context& context)
  : Destination(logger)
  , OutputContext(context)
{
}

WStream::~WStream()
{
#if !defined(__GNUC__) && !defined(__GNUG__)
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
  auto s = convert.to_bytes(str());
#else
  auto s = Logme::ToStdString(str());
#endif

  Destination->Log(OutputContext, s.c_str());
}
