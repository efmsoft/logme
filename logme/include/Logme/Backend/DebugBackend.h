#pragma once

#include "Backend.h"
#include <memory>

namespace Logme
{
  struct DebugBackend : public Backend
  {
    DebugBackend(ChannelPtr owner);

    void Display(Context& context, const char* line) override;
  };
}
