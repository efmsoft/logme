#pragma once

#include <list>
#include <mutex>

#include <Logme/Backend/Backend.h>

namespace Logme
{
  struct RingBufferBackendConfig : public BackendConfig
  {
    size_t MaxItems;
    enum { MAX_ITEMS_DEFAULT = 1000 };

    LOGMELNK RingBufferBackendConfig();

    LOGMELNK bool Parse(const Json::Value* po) override;
  };

  struct RingBufferBackend : public Backend
  {
    std::mutex Lock;
    std::list<std::string> Ring;
    RingBufferBackendConfig Config;

    constexpr static const char* TYPE_ID = "RingBufferBackend";

    LOGMELNK RingBufferBackend(ChannelPtr owner);

    LOGMELNK void Display(Context& context, const char* line) override;

    LOGMELNK void Clear();
    LOGMELNK void Append(const char* str, int nc);
    LOGMELNK std::string Join();  

    LOGMELNK BackendConfigPtr CreateConfig() override;
    LOGMELNK bool ApplyConfig(BackendConfigPtr c) override;
  };
}
