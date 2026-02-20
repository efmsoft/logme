#include <stdint.h>
#include <string>

#include <Logme/Utils.h>

#include "StringHelpers.h"

std::string Logme::DumpBuffer(const void* buffer, size_t n, size_t offs, size_t lineLimit)
{
  std::string output;

  const int maxCols = 16;
  const int maxLines = lineLimit > 0 ? int(lineLimit) : 65536;
  const size_t maxSize = size_t(maxCols) * maxLines;

  uint8_t* p = (uint8_t*)buffer;
  n = std::min(n, maxSize);

  std::string offset(offs, ' ');

  size_t pos = 0;
  for (int line = 0; line < maxLines && pos < n; line++)
  {
    std::string txt("| ");
    std::string str;
    for (int col = 0; col < maxCols && pos < n; col++)
    {
      uint8_t b = *p++;
      pos++;

      char rune[16]{};
      sprintf_s(rune, sizeof(rune), "%02x ", b);
      str += rune;

      if (b < ' ' || b > '~')
      {
        b = '.';
        txt += b;
      }
      else
      {
        sprintf_s(rune, sizeof(rune), "%c", b);
        txt += rune;
      }
    }

    size_t e = 3ULL * maxCols;
    while (str.length() < e)
      str += ' ';

    std::string out(offset);
    out += " ";
    out += str;
    out += txt;

    if (!output.empty())
      output += '\n';

    output += out.c_str();
  }

  return output;
}
