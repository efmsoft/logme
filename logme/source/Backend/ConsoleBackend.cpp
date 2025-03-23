#include <Logme/AnsiColorEscape.h>
#include <Logme/Backend/ConsoleBackend.h>
#include <Logme/Channel.h>
#include <Logme/Colorizer.h>

#include <iostream>
#include <cstdio>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#define _isatty isatty
#define _fileno fileno
#endif

using namespace Logme;

ConsoleBackend::ConsoleBackend(ChannelPtr owner)
  : Backend(owner, TYPE_ID)
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

std::ostream& ConsoleBackend::GetOutputStream(Context& context)
{
  auto flags = Owner->GetFlags();

  switch (flags.Console)
  {
    case ConsoleStream::STREAM_ALL2COUT:
      return std::cout;

    case ConsoleStream::STREAM_ALL2CERR:
      return std::cerr;

    case ConsoleStream::STREAM_WARNCERR:
      return context.ErrorLevel >= Level::LEVEL_WARN ? std::cerr : std::cout;

    case ConsoleStream::STREAM_ERRCERR:
      return context.ErrorLevel >= Level::LEVEL_ERROR? std::cerr : std::cout;

    case ConsoleStream::STREAM_CERRCERR:
      return context.ErrorLevel >= Level::LEVEL_CRITICAL? std::cerr : std::cout;
  }
  return std::cout;
}

static bool IsTerminalStream(std::ostream& stream)
{
  if (&stream == &std::cerr)
    return !!_isatty(_fileno(stderr));

  return !!_isatty(_fileno(stdout));
}

void ConsoleBackend::Display(Context& context, const char* line)
{
  OutputFlags flags = Owner->GetFlags();
  if (context.Ovr)
  {
    flags.Value |= context.Ovr->Add.Value;
    flags.Value &= ~context.Ovr->Remove.Value;
  }

  int nc;
  const char* buffer = context.Apply(Owner, flags, line, nc);
  const char* escape = nullptr;

  if (flags.Highlight)
    escape = GetEscapeSequence(context.ErrorLevel);

  auto& stream = GetOutputStream(context);
  if (IsTerminalStream(stream) && escape)
  {
    Colorizer colorizer(false);
    colorizer.Escape(escape);

    stream << buffer;
  }
  else
    stream << buffer;
}
