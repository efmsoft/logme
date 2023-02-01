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
    const char* Type;

  public:
    Backend(ChannelPtr owner, const char* type);
    virtual ~Backend();

    virtual void Display(Context& context, const char* line) = 0;
    const char* GetType() const;
  };

  typedef std::shared_ptr<Backend> BackendPtr;
  typedef std::vector<BackendPtr> BackendArray;
}
