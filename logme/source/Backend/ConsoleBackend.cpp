#include <Logme/AnsiColorEscape.h>
#include <Logme/Backend/ConsoleBackend.h>
#include <Logme/Channel.h>
#include <Logme/Colorizer.h>

#include <stdio.h>

using namespace Logme;

ConsoleBackend::ConsoleBackend(ChannelPtr owner)
  : Backend(owner)
{
}

const char* ConsoleBackend::GetEscapeSequence(enum Level level)
{
  if (level < Level::LEVEL_WARN)
    return nullptr;

  if (level == Level::LEVEL_WARN)
    return ANSI_YELLOW;

  return ANSI_LIGHT_RED;
}

void ConsoleBackend::Display(Context& context, const char* line)
{
  auto flags = Owner->GetFlags();

  int nc;
  const char* buffer = context.Apply(flags, line, nc);

  const char* escape = nullptr;
  if (flags.Highlight)
    escape = GetEscapeSequence(context.ErrorLevel);

  if (escape)
  {
    Colorizer colorizer(false);
    colorizer.Escape(escape);

    printf("%s", buffer);
  }
  else
    printf("%s", buffer);
}
