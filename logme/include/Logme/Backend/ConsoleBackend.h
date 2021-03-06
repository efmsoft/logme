#pragma once

#include <Logme/Backend/Backend.h>
#include <iostream>

namespace Logme
{
  struct ConsoleBackend : public Backend
  {
    ConsoleBackend(ChannelPtr owner);

    void Display(Context& context, const char* line) override;
    const char* GetEscapeSequence(Level level);

    std::ostream& GetOutputStream(Context& context);
  };
}
