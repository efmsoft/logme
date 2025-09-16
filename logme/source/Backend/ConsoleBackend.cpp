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

FILE* ConsoleBackend::GetOutputStream(Context& context)
{
  OutputFlags flags = Owner->GetFlags();

  switch (flags.Console)
  {
    case ConsoleStream::STREAM_ALL2COUT:
      return stdout;

    case ConsoleStream::STREAM_ALL2CERR:
      return stderr;

    case ConsoleStream::STREAM_WARNCERR:
      return context.ErrorLevel >= Level::LEVEL_WARN ? stderr : stdout;

    case ConsoleStream::STREAM_ERRCERR:
      return context.ErrorLevel >= Level::LEVEL_ERROR? stderr : stdout;

    case ConsoleStream::STREAM_CERRCERR:
      return context.ErrorLevel >= Level::LEVEL_CRITICAL? stderr : stdout;
  }
  return stdout;
}

static bool IsTerminalStream(FILE* stream)
{
  if (stream == stderr)
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

  FILE* stream = GetOutputStream(context);
  if (IsTerminalStream(stream) && escape)
  {
    static std::mutex ColorizerLock;
    std::lock_guard guard(ColorizerLock);

    Colorizer colorizer(false);
    colorizer.Escape(escape);

    fputs(buffer, stream);
  }
  else
  {
    fputs(buffer, stream);
  }
}
