#pragma once

#include <Logme/Types.h>

#include <memory>
#include <vector>

namespace Json
{
  class Value;
}

namespace Logme
{
  class Channel;
  struct Context;

  typedef std::shared_ptr<Channel> ChannelPtr;
  struct BackendConfig;

  struct BackendConfig
  {
    std::string Type;

    virtual ~BackendConfig();
    virtual bool Parse(const Json::Value* po);
  };

  typedef std::shared_ptr<BackendConfig> BackendConfigPtr;
  typedef std::vector<BackendConfigPtr> BackendConfigArray;

  struct Backend;
  typedef std::shared_ptr<Backend> BackendPtr;
  typedef std::vector<BackendPtr> BackendArray;

  struct Backend
  {
    ChannelPtr Owner;
    const char* Type;

  public:
    Backend(ChannelPtr owner, const char* type);
    virtual ~Backend();

    virtual void Display(Context& context, const char* line) = 0;
    virtual BackendConfigPtr CreateConfig();
    virtual bool ApplyConfig(BackendConfigPtr c);

    const char* GetType() const;

    static BackendPtr Create(const char* type, ChannelPtr owner);
  };
}
