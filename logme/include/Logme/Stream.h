#pragma once

#include <Logme/Context.h>
#include <Logme/Types.h>

#include <memory>
#include <sstream>

namespace Logme
{
  class Stream : public std::stringstream
  {
    LoggerPtr Destination;
    const Context& OutputContext;
    OverridePtr Ovr;

  public:
    LOGMELNK Stream(LoggerPtr logger, const Context& context, OverridePtr ovr = OverridePtr());
    Stream(const Stream&) = delete;
    Stream& operator=(const Stream&) = delete;

    Stream(Stream&&) = delete;
    Stream& operator=(Stream&&) = delete;
    LOGMELNK ~Stream();
  };
}
