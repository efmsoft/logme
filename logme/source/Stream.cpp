#include <Logme/Logger.h>
#include <codecvt>

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

/*
Stream& Stream::operator<<(const wchar_t* wstr)
{
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
  auto s = convert.to_bytes(wstr);

  std::stringstream::operator<<(s.c_str());
  return *this;
}
*/
WStream::WStream(LoggerPtr logger, const Context& context)
  : Destination(logger)
  , OutputContext(context)
{
}

WStream::~WStream()
{
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
  auto s = convert.to_bytes(str());

  Destination->Log(OutputContext, s.c_str());
}
/*
WStream& WStream::operator<<(const char* str)
{
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
  auto s = convert.from_bytes(str);

  std::wstringstream::operator<<(s.c_str());
  return *this;
}
*/
