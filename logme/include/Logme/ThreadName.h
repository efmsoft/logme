#pragma once

#include <memory>
#include <optional>
#include <string>

#include <Logme/Types.h>

namespace Logme
{
  class Channel;
  typedef std::shared_ptr<Channel> ChannelPtr;

  class ThreadName
  {
    ChannelPtr PCH;
    std::optional<std::string> PreviousName;
    bool Log;

  public:
    LOGMELNK ThreadName(ChannelPtr pch, const char* name, bool log = true);
    LOGMELNK ThreadName(ChannelPtr pch, const std::string& name, bool log = true);
    LOGMELNK ~ThreadName();
  
  private:
    void Initialize(const char* name);
  };
}