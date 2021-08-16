#pragma once

#include <Logme/Backend/Backend.h>

namespace Logme
{
  struct ConsoleBackend : public Backend
  {
    ConsoleBackend(ChannelPtr owner);

    void Display(Context& context, const char* line) override;
    const char* GetEscapeSequence(Level level);
  };
}
