#include <Logme/Colorizer.h>
#include <Logme/AnsiColorEscape.h>

#include <stdlib.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

using namespace Logme;

#ifdef _WIN32
static WORD ColorFlags[] =
{
  0,                                        // black
  FOREGROUND_RED,                           // red
  FOREGROUND_GREEN,                         // green
  FOREGROUND_RED | FOREGROUND_GREEN,        // yellow
  FOREGROUND_BLUE,                          // blue
  FOREGROUND_RED | FOREGROUND_BLUE,         // magenta
  FOREGROUND_GREEN | FOREGROUND_BLUE,       // cyan
  FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, // white
};

bool VTModeEnabled = false;
#endif

#ifndef _WIN32
static bool IsTTY(FILE* stream)
{
#ifdef _WIN32
  return (_isatty(_fileno(stream)) != 0);
#else
  return (isatty(fileno(stream)) != 0);
#endif
}
#endif

Colorizer::Colorizer(bool isStdErr)
#ifdef _WIN32
  : Sbi{}
#endif
{
#ifndef _WIN32
  Stream = isStdErr ? stderr : stdout;
  if (!IsTTY(Stream))
    Stream = 0;
#else
  LastAttr = 0;
  Output = GetStdHandle(isStdErr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
  if (Output == INVALID_HANDLE_VALUE)
    Output = 0;

  if (Output)
  {
    if (!GetConsoleScreenBufferInfo(Output, &Sbi))
      Output = 0;                           // not a console
    else
      LastAttr = Sbi.wAttributes;
  }
#endif
}

Colorizer::~Colorizer()
{
  Escape();                                 // Back to defaults colors
}

bool Colorizer::VTMode()
{
#ifdef _WIN32
  return VTModeEnabled;
#else
  return true;
#endif
}

bool Colorizer::EnableVTMode()
{
#ifdef _WIN32
  if (VTModeEnabled)
    return true;

  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hOut == INVALID_HANDLE_VALUE)
    return false;

  DWORD dwMode = 0;
  if (!GetConsoleMode(hOut, &dwMode))
    return false;

  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    
  if (!SetConsoleMode(hOut, dwMode))
    return false;

  VTModeEnabled = true;
  return true;
#else
  return true;
#endif
}

void Colorizer::Escape(const char* escape)
{
#ifndef _WIN32
  if (Stream)
    fputs(escape ? escape : ANSI_RESET, Stream);
#else
  if (!Output)
    return;

  int attr, fg, bg;
  escape = escape ? escape : ANSI_RESET;

  if (!ParseSequence(escape, attr, fg, bg))
    return;

  if (!attr && fg == -1 && bg == -1)
  {
    SetConsoleTextAttribute(Output, Sbi.wAttributes);
    return;
  }

  WORD textAttr = LastAttr;
  if (fg >= 30 && fg <= 37)
  {
    textAttr = (textAttr & ~0xF) | ColorFlags[fg - 30];
    if (attr == 1)
      textAttr |= FOREGROUND_INTENSITY;
  }

  if (bg >= 40 && bg <= 47)
  {
    textAttr = (textAttr & ~0xF0) | (ColorFlags[bg - 40] << 4);
    if (attr == 1)
      textAttr |= BACKGROUND_INTENSITY;
  }

  SetConsoleTextAttribute(Output, textAttr);
  LastAttr = textAttr;
#endif
}

bool Colorizer::ParseSequence(const char*& escape, int& attr, int& fg, int& bg)
{
  attr = 0;
  fg = -1;
  bg = -1;

  if (!escape || escape[0] != '\x1b' || escape[1] != '[')
    return false;

  for (const char* p = escape + 2; *p;)
  {
    char* e;
    int v = (int)strtol(p, &e, 10);
    if (v >= 0 && v <= 8)
      attr = v;
    else
    if (v >= 30 && v <= 37)
      fg = v;
    else
    if (v >= 40 && v <= 47)
      bg = v;
    else
      return false;

    if (*e == ';')
    {
      p = e + 1;
      continue;
    }

    if (*e == 'm')
    {
      p = e + 1;
      escape = p;
      break;
    }

    return false;
  }

  return true;
}
