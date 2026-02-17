#pragma once

#include <Logme/Backend/Backend.h>
#include <Logme/Channel.h>

#include <string>
#include <vector>

namespace Logme
{
  enum class BufferBackendPolicy
  {
    STOP_APPENDING,
    DELETE_OLDEST
  };

  struct BufferBackendConfig : public BackendConfig
  {
    enum { MAX_SIZE_DEFAULT = 4 * 1024 * 1024 };

    size_t MaxSize;
    BufferBackendPolicy Policy;

    LOGMELNK BufferBackendConfig();

    LOGMELNK bool Parse(const Json::Value* po) override;
  };

  struct BufferBackend : public Logme::Backend
  {
    enum { GROW = 4 * 1024 };
    std::vector<char> Buffer;
    BufferBackendConfig Config;

    constexpr static const char* TYPE_ID = "BufferBackend";

  public:
    LOGMELNK BufferBackend(Logme::ChannelPtr owner);

    LOGMELNK void Clear();
    LOGMELNK void Display(Logme::Context& context, const char* line) override;
    LOGMELNK void Append(const BufferBackend& bb);
    LOGMELNK void Append(const char* str, int nc);

    LOGMELNK BackendConfigPtr CreateConfig() override;
    LOGMELNK bool ApplyConfig(BackendConfigPtr c) override;
  };
}