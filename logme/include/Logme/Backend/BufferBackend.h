#pragma once

#include <Logme/Backend/Backend.h>
#include <Logme/Channel.h>

#include <string>
#include <vector>

namespace Logme
{
  struct BufferBackend : public Logme::Backend
  {
    enum { GROW = 4 * 1024 };
    std::vector<char> Buffer;

    constexpr static const char* TYPE_ID = "BufferBackend";

  public:
    LOGMELNK BufferBackend(Logme::ChannelPtr owner);

    LOGMELNK void Clear();
    LOGMELNK void Display(Logme::Context& context, const char* line) override;
    LOGMELNK void Insert(const std::vector<char>& buffer);
  };
}