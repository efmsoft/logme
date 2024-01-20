#pragma once

#include <Logme/Backend/Backend.h>
#include <memory>

namespace Logme
{
  struct DebugBackend : public Backend
  {
    constexpr static const char* TYPE_ID = "DebugBackend";

    LOGMELNK DebugBackend(ChannelPtr owner);

    LOGMELNK void Display(Context& context, const char* line) override;
  };
}
