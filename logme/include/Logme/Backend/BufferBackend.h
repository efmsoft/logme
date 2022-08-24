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

  public:
    BufferBackend(Logme::ChannelPtr owner);

    void Clear();
    void Display(Logme::Context& context, const char* line) override;
  };
}