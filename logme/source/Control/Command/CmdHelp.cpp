#include "../CommandRegistrar.h"

using namespace Logme;

COMMAND_DESCRIPTOR("help", CommandHelp);

static size_t FindHelpSplit(const std::string& line)
{
  for (size_t i = 0; i < line.size(); ++i)
  {
    if (line[i] == '\t')
      return i;

    if (line[i] == ' ' && (i + 1) < line.size() && line[i + 1] == ' ')
    {
      size_t j = i + 2;
      while (j < line.size() && line[j] == ' ')
        ++j;

      if (j < line.size())
        return i;
    }
  }

  return std::string::npos;
}

static void AlignHelpColumns(std::string& s)
{
  StringArray lines;
  lines.reserve(64);

  size_t start = 0;
  while (start < s.size())
  {
    size_t end = s.find('\n', start);
    if (end == std::string::npos)
      end = s.size();

    std::string line = s.substr(start, end - start);
    if (!line.empty() && line.back() == '\r')
      line.pop_back();

    lines.push_back(line);
    start = (end < s.size()) ? (end + 1) : end;
  }

  size_t maxLeft = 0;
  for (const std::string& line : lines)
  {
    size_t split = FindHelpSplit(line);
    if (split == std::string::npos)
      continue;

    std::string left = TrimSpaces(line.substr(0, split));
    std::string right = TrimSpaces(line.substr(split));
    if (left.empty() || right.empty())
      continue;

    if (left.size() > maxLeft)
      maxLeft = left.size();
  }

  if (maxLeft == 0)
    return;

  std::string out;
  out.reserve(s.size() + lines.size() * 8);

  for (size_t i = 0; i < lines.size(); ++i)
  {
    const std::string& line = lines[i];
    size_t split = FindHelpSplit(line);
    if (split == std::string::npos)
    {
      out += line;
      out += "\n";
      continue;
    }

    std::string left = TrimSpaces(line.substr(0, split));
    std::string right = TrimSpaces(line.substr(split));
    if (left.empty() || right.empty())
    {
      out += line;
      out += "\n";
      continue;
    }

    out += left;
    out.append((maxLeft - left.size()) + 2, ' ');
    out += right;
    out += "\n";
  }

  s.swap(out);
}

static bool CommandHelp(Logme::StringArray& arr, std::string& response)
{
  (void)arr;

  response +=
    "backend [--channel name] --add type           Add backend to a channel\n"
    "backend [--channel name] --delete type        Delete backend from a channel\n"
    "channel --create name                         Create a channel\n"
    "channel --delete name                         Delete a channel (not default)\n"
    "channel --disable name                        Disable a channel\n"
    "channel --enable name                         Enable a channel\n"
    "channel [name]                                Display channel information\n"
    "flags [--channel name] [flag[=value] ...]      Set or display channel flags\n"
    "help                                           Print this help text\n"
    "level [--channel name] [level]                 Get or set channel level\n"
    "list                                           List channels\n"
    "subsystem --block-reported                     Block reported subsystems\n"
    "subsystem --report name                        Add reported subsystem\n"
    "subsystem --unblock-reported                   Unblock reported subsystems\n"
    "subsystem --unreport name                      Remove reported subsystem\n"
    "subsystem [name]                               Query reported subsystem\n"
  ;

  SortLines(response);
  AlignHelpColumns(response);
  return true;
}

