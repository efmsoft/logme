#include <atomic>
#include <functional>
#include <vector>

#include <Logme/OutputFlags.h>
#include <Logme/Utils.h>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

using namespace Logme;


namespace
{
  struct OutputFieldNames
  {
    std::string Names[OUTPUT_FIELD_COUNT];

    OutputFieldNames()
    {
      Names[OUTPUT_FIELD_TIMESTAMP] = "timestamp";
      Names[OUTPUT_FIELD_LEVEL] = "level";
      Names[OUTPUT_FIELD_PROCESS_ID] = "process_id";
      Names[OUTPUT_FIELD_THREAD_ID] = "thread_id";
      Names[OUTPUT_FIELD_CHANNEL] = "channel";
      Names[OUTPUT_FIELD_SUBSYSTEM] = "subsystem";
      Names[OUTPUT_FIELD_FILE] = "file";
      Names[OUTPUT_FIELD_LINE] = "line";
      Names[OUTPUT_FIELD_METHOD] = "method";
      Names[OUTPUT_FIELD_MESSAGE] = "message";
      Names[OUTPUT_FIELD_DURATION] = "duration";
    }
  };

  const OutputFieldNames* CreateDefaultFieldNames()
  {
    return new OutputFieldNames();
  }

  std::atomic<const OutputFieldNames*> CurrentFieldNames(CreateDefaultFieldNames());

  bool IsValidField(OutputField field)
  {
    return field >= 0 && field < OUTPUT_FIELD_COUNT;
  }
}

void Logme::SetOutputFieldName(OutputField field, const char* name)
{
  if (!IsValidField(field) || name == nullptr || *name == '\0')
    return;

  const OutputFieldNames* current = CurrentFieldNames.load(std::memory_order_acquire);
  OutputFieldNames* updated = new OutputFieldNames(*current);
  updated->Names[field] = name;
  CurrentFieldNames.store(updated, std::memory_order_release);
}

void Logme::SetOutputFieldNames(const OutputFieldNameMap& names)
{
  const OutputFieldNames* current = CurrentFieldNames.load(std::memory_order_acquire);
  OutputFieldNames* updated = new OutputFieldNames(*current);

  for (const auto& item : names)
  {
    if (IsValidField(item.first) && !item.second.empty())
      updated->Names[item.first] = item.second;
  }

  CurrentFieldNames.store(updated, std::memory_order_release);
}

const char* Logme::GetOutputFieldName(OutputField field)
{
  const OutputFieldNames* current = CurrentFieldNames.load(std::memory_order_acquire);
  if (!IsValidField(field))
    return "";

  return current->Names[field].c_str();
}

void Logme::ResetOutputFieldNames()
{
  CurrentFieldNames.store(CreateDefaultFieldNames(), std::memory_order_release);
}

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
  APPEND_FLAG(Format);

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
