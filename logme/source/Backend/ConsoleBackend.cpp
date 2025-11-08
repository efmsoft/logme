#include <cstdio>
#include <cstring>
#include <iostream>

#include <Logme/AnsiColorEscape.h>
#include <Logme/Backend/ConsoleBackend.h>
#include <Logme/Channel.h>
#include <Logme/Colorizer.h>
#include <Logme/Logger.h>

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
  if (owner)
  {
    auto logger = owner->GetOwner();
    if (logger->GetEnableVTMode())
      Colorizer::EnableVTMode();
  }
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

static void PrintWithAnsiSegments(
  FILE* stream
  , const char* text
  , const char* defaultEscape
)
{
  if (!text || !*text)
    return;

  bool isStdErr = (stream == stderr);

  Colorizer colorizer(isStdErr);

  // If VT mode is enabled we do not parse escapes ourselves.
  if (Colorizer::VTMode())
  {
    if (defaultEscape && *defaultEscape)
      colorizer.Escape(defaultEscape);

    fputs(text, stream);
    return;
  }

  const char* p = text;
  const char* escPos = nullptr;
  bool defaultApplied = false;

  while ((escPos = std::strchr(p, '\x1b')) != nullptr)
  {
    if (escPos > p)
    {
      size_t len = static_cast<size_t>(escPos - p);

      if (defaultEscape && !defaultApplied)
      {
        colorizer.Escape(defaultEscape);
        defaultApplied = true;
      }

      if (len > 0)
        fwrite(p, 1, len, stream);
    }

    const char* seq = escPos;
    int attr = 0;
    int fg = -1;
    int bg = -1;

    if (Colorizer::ParseSequence(seq, attr, fg, bg))
    {
      colorizer.Escape(escPos);
      defaultApplied = false;
      p = seq;
      continue;
    }
    else
    {
      fputc('\x1b', stream);
      p = escPos + 1;
      continue;
    }
  }

  if (*p)
  {
    if (defaultEscape && !defaultApplied)
    {
      colorizer.Escape(defaultEscape);
      defaultApplied = true;
    }

    fputs(p, stream);
  }

  // Colorizer dtor will restore original console attributes.
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

  const bool hasAnsi = (std::strchr(buffer, '\x1b') != nullptr);

  FILE* stream = GetOutputStream(context);
  if (IsTerminalStream(stream) && (escape || hasAnsi))
  {
    static std::mutex ColorizerLock;
    std::lock_guard guard(ColorizerLock);

    if (hasAnsi)
    {
      PrintWithAnsiSegments(stream, buffer, escape);
    }
    else if (escape)
    {
      Colorizer colorizer(false);
      colorizer.Escape(escape);

      fputs(buffer, stream);
    }
  }
  else
  {
    fputs(buffer, stream);
  }
}
