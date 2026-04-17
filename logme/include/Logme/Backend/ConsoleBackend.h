#pragma once

#include <Logme/Backend/Backend.h>
#include <iostream>

namespace Logme
{
  struct ConsoleBackend : public Backend
  {
    constexpr static const char* TYPE_ID = "ConsoleBackend";

    LOGMELNK ConsoleBackend(ChannelPtr owner);

    LOGMELNK void Display(Context& context) override;
    LOGMELNK const char* GetEscapeSequence(Level level);

    LOGMELNK FILE* GetOutputStream(Context& context);
  };
}
