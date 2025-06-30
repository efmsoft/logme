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

    LOGMELNK virtual ~BackendConfig();
    LOGMELNK virtual bool Parse(const Json::Value* po);
  };

  typedef std::shared_ptr<BackendConfig> BackendConfigPtr;
  typedef std::vector<BackendConfigPtr> BackendConfigArray;

  struct Backend;
  typedef std::shared_ptr<Backend> BackendPtr;
  typedef std::vector<BackendPtr> BackendArray;

  struct Backend : public std::enable_shared_from_this<Backend>
  {
    ChannelPtr Owner;
    const char* Type;
    bool Freezed;

  public:
    LOGMELNK Backend(ChannelPtr owner, const char* type);
    LOGMELNK virtual ~Backend();

    LOGMELNK virtual void Display(Context& context, const char* line) = 0;
    LOGMELNK virtual BackendConfigPtr CreateConfig();
    LOGMELNK virtual bool ApplyConfig(BackendConfigPtr c);
    LOGMELNK virtual bool IsIdle() const;
    LOGMELNK virtual void Freeze();
    LOGMELNK virtual void Flush();

    LOGMELNK const char* GetType() const;

    LOGMELNK static BackendPtr Create(const char* type, ChannelPtr owner);
    LOGMELNK virtual std::string FormatDetails();
  };
}
