#pragma once

#include <Logme/Context.h>
#include <Logme/Types.h>

#include <memory>
#include <sstream>

namespace Logme
{
  class Stream 
    : public std::stringstream
  {
    LoggerPtr Destination;
    const Context& OutputContext;

  public:
    Stream(LoggerPtr logger, const Context& context);
    ~Stream();

    //using std::stringstream::operator<<;
    //Stream& operator<<(const wchar_t*);
  };

  class WStream
    : public std::wstringstream
  {
    LoggerPtr Destination;
    const Context& OutputContext;

  public:
    WStream(LoggerPtr logger, const Context& context);
    ~WStream();

    //using std::wstringstream::operator<<;
    //WStream& operator<<(const char*);
  };
}
