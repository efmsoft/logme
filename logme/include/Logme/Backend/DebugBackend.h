#pragma once

#include <memory>

#include <Logme/Backend/Backend.h>

namespace Logme
{
  struct DebugBackend : public Backend
  {
    constexpr static const char* TYPE_ID = "DebugBackend";

    LOGMELNK DebugBackend(ChannelPtr owner);

    LOGMELNK void Display(Context& context) override;
  };
}
