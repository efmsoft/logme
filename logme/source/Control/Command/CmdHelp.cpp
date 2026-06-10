#include "../CommandRegistrar.h"

#include <Logme/version.h>
#include <Logme/Logger.h>

using namespace Logme;

static Logme::CommandDescriptor HelpDescriptor("help", Logger::CommandHelp);
static Logme::CommandRegistrar HelpRegistrar(&HelpDescriptor);
static Logme::CommandDescriptor VersionDescriptor("version", Logger::CommandVersion);
static Logme::CommandRegistrar VersionRegistrar(&VersionDescriptor);

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

bool Logger::CommandVersion(Logme::StringArray& arr, std::string& response)
{
  (void)arr;

  response += "Logme version: ";
  response += LOGME_VERSION_STRING;
  response += "\n";
  response += "Control protocol: 1\n";
  return true;
}

bool Logger::CommandHelp(Logme::StringArray& arr, std::string& response)
{
  (void)arr;

  response +=
    "backend [--channel name] --add type [options] Add backend to a channel\n"
    "backend [--channel name] --delete type        Delete backend from a channel\n"
    "backend option --async                       Enable async output when backend supports it\n"
    "backend option --file path                   FileBackend/SharedFileBackend: set output file\n"
    "backend option --append                      FileBackend only: append to output file\n"
    "backend option --overwrite                   FileBackend only: overwrite output file\n"
    "backend option --max-size size               FileBackend/SharedFileBackend/BufferBackend maximum size\n"
    "backend option --daily-rotation              FileBackend only: rotate by day\n"
    "backend option --no-daily-rotation           FileBackend only: disable daily rotation\n"
    "backend option --max-parts count             FileBackend only: keep rotated file parts\n"
    "backend option --timeout interval            SharedFileBackend only: lock timeout\n"
    "backend option --policy policy               BufferBackend only: stop-appending or delete-oldest\n"
    "backend option --max-items count             RingBufferBackend only: keep last record count\n"
    "channel [name]                                Display channel information\n"
    "channel --create name                         Create a channel\n"
    "channel --delete name                         Delete a channel (not default)\n"
    "channel --enable name                         Enable a channel\n"
    "channel --disable name                        Disable a channel\n"
    "channel --bind source target                  Link source channel to target channel\n"
    "channel --unbind name                         Remove channel link\n"
    "channel --error name                          Set error channel\n"
    "channel --clear-error                         Clear error channel\n"
    "flags [--channel name] [flag[=value] ...]      Set or display channel flags\n"
    "help                                           Print this help text\n"
    "level [--channel name] [level]                 Get or set channel level\n"
    "list                                           List channels\n"
    "logs [--info|--tree [path]|--tail path [bytes]] Browse log files under home directory\n"
    "overview                                       Display runtime logging summary\n"
    "subsystem                                      Display subsystem filters\n"
    "subsystem --block name                         Add blocked subsystem\n"
    "subsystem --unblock name                       Remove blocked subsystem\n"
    "subsystem --allow name                         Add allowed subsystem\n"
    "subsystem --disallow name                      Remove allowed subsystem\n"
    "subsystem --check name                         Query subsystem filters\n"
    "subsystem --clear                              Clear all subsystem filters\n"
    "subsystem --clear-blocked                      Clear blocked subsystem list\n"
    "subsystem --clear-allowed                      Clear allowed subsystem list\n"
    "trace [list|stat|stats] [pattern]              List trace points and counters\n"
    "trace enable pattern                           Enable trace points by module:function:line wildcard\n"
    "trace disable pattern                          Disable trace points by module:function:line wildcard\n"
    "trace reset [pattern]                          Reset trace point counters\n"
    "version                                        Display logme and control protocol version"
  ;

  SortLines(response);
  AlignHelpColumns(response);
  return true;
}

