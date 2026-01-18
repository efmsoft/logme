#include <functional>
#include <vector>

#include <Logme/OutputFlags.h>
#include <Logme/Utils.h>

using namespace Logme;

OutputFlags::OutputFlags()
  : Value(0)
{
  Timestamp = true;
  Signature = true;
  Method = true;
  ErrorPrefix = true;
  Highlight = true;
  Eol = true;
  ThreadTransition = true;
}

typedef std::function<std::string()> FormatFlagFunc;
static std::string FormatFlag(const char* name, FormatFlagFunc func = FormatFlagFunc())
{
  if (func)
  {
    std::string arg = func();
    if (arg.empty())
      return std::string(name);

    return std::string(name) + ":" + arg;
  }

  return std::string(name);
}

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

#define APPEND_FLAG(name, ...) \
  if (name) \
    flags.push_back(FormatFlag(#name, ## __VA_ARGS__))

std::string OutputFlags::TimestampType() const
{
  TimeFormat fmt = (TimeFormat)Timestamp;
  switch (fmt)
  {
  case TimeFormat::TIME_FORMAT_NONE: return "";
  case TimeFormat::TIME_FORMAT_LOCAL: return "Local";
  case TimeFormat::TIME_FORMAT_TZ: return "TZ";
  case TimeFormat::TIME_FORMAT_UTC: return "UTC";
  default: return "Unknown";
  }
}

std::string OutputFlags::LocationType() const
{
  Detality det = (Detality)Location;
  switch (det)
  {
  case Detality::DETALITY_NONE: return "";
  case Detality::DETALITY_SHORT: return "Short";
  case Detality::DETALITY_FULL: return "Full";
  default: return "Unknown";
  }
}
std::string OutputFlags::ConsoleType() const
{
  switch (Console)
  {
  case STREAM_ALL2COUT: return "cout";
  case STREAM_WARNCERR: return "warncerr";
  case STREAM_ERRCERR: return "errcerr";
  case STREAM_CERRCERR: return "cerrcerr";
  case STREAM_ALL2CERR: return "cerr";
  default: return "Unknown";
  }
}

std::string OutputFlags::ToString(const char* separator, bool brackets) const
{
  std::vector<std::string> flags;

  APPEND_FLAG(Timestamp, [this]() {return TimestampType(); });
  APPEND_FLAG(Signature);
  APPEND_FLAG(Location, [this]() {return LocationType(); });
  APPEND_FLAG(Method);
  APPEND_FLAG(Eol);
  APPEND_FLAG(ErrorPrefix);
  APPEND_FLAG(Duration);
  APPEND_FLAG(ThreadID);
  APPEND_FLAG(ProcessID);
  APPEND_FLAG(Channel);
  APPEND_FLAG(Highlight);
  APPEND_FLAG(Console, [this]() {return ConsoleType(); });
  APPEND_FLAG(DisableLink);
  APPEND_FLAG(ThreadTransition);
  APPEND_FLAG(Subsystem);

  std::string result = Join(flags, separator);
  if (brackets && !result.empty())
  {
    result = "[" + result + "]";
  }

  return result;
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
