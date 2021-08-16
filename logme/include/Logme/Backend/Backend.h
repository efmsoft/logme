#pragma once

#include <Logme/Types.h>

#include <memory>
#include <vector>

namespace Logme
{
  class Channel;
  struct Context;

  typedef std::shared_ptr<Channel> ChannelPtr;

  struct Backend
  {
    ChannelPtr Owner;

  public:
    Backend(ChannelPtr owner);
    virtual ~Backend();

    virtual void Display(Context& context, const char* line) = 0;
  };

  typedef std::shared_ptr<Backend> BackendPtr;
  typedef std::vector<BackendPtr> BackendArray;
}
