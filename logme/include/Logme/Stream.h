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

  public:
    LOGMELNK Stream(LoggerPtr logger, const Context& context);
    LOGMELNK Stream(const Stream& src);
    LOGMELNK ~Stream();
  };
}
