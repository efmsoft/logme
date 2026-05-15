#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>

#include <Logme/TracePoint.h>

using namespace Logme;

namespace
{
  std::string ToLower(std::string s)
  {
    std::transform(
      s.begin()
      , s.end()
      , s.begin()
      , [](unsigned char c)
      {
        return (char)std::tolower(c);
      }
    );

    return s;
  }

  bool WildcardMatchInternal(
    const char* text
    , const char* pattern
  )
  {
    const char* star = nullptr;
    const char* retry = nullptr;

    while (*text)
    {
      if (*pattern == '?' || *pattern == *text)
      {
        ++text;
        ++pattern;
        continue;
      }

      if (*pattern == '*')
      {
        star = pattern++;
        retry = text;
        continue;
      }

      if (star)
      {
        pattern = star + 1;
        text = ++retry;
        continue;
      }

      return false;
    }

    while (*pattern == '*')
      ++pattern;

    return *pattern == 0;
  }

  std::string MakeTracePointKey(const TracePoint& point)
  {
    std::string key;
    key.reserve(128);
    key += point.Module ? point.Module : "";
    key += ":";
    key += point.Method ? point.Method : "";
    key += ":";
    key += std::to_string(point.Line);
    return key;
  }
}

void Detail::RegisterTracePointOnce(TracePoint& point)
{
  if (point.Registered)
    return;

  if (Instance)
    Instance->RegisterTracePoint(point);
}

bool Detail::TracePointMatch(
  const TracePoint& point
  , const std::string& pattern
)
{
  if (pattern.empty())
    return true;

  std::string key = ToLower(MakeTracePointKey(point));
  std::string pat = ToLower(pattern);
  return WildcardMatchInternal(key.c_str(), pat.c_str());
}

Override& Detail::TracePointOverride()
{
  static Override ovr;
  ovr.Add.Location = DETALITY_SHORT;
  ovr.Add.Method = true;
  return ovr;
}

void Logger::RegisterTracePoint(TracePoint& point)
{
  if (point.Registered)
    return;

  std::lock_guard guard(TracePointLock);

  if (point.Registered)
    return;

  point.Next = TracePoints;
  TracePoints = &point;
  point.Registered = true;
}

size_t Logger::SetTracePointsEnabled(
  const std::string& pattern
  , bool enabled
)
{
  std::lock_guard guard(TracePointLock);

  size_t count = 0;
  for (TracePoint* point = TracePoints; point; point = point->Next)
  {
    if (!Detail::TracePointMatch(*point, pattern))
      continue;

    point->Enabled = enabled;
    ++count;
  }

  return count;
}

size_t Logger::ResetTracePointCounters(const std::string& pattern)
{
  std::lock_guard guard(TracePointLock);

  size_t count = 0;
  for (TracePoint* point = TracePoints; point; point = point->Next)
  {
    if (!Detail::TracePointMatch(*point, pattern))
      continue;

    point->ResetCounter();
    ++count;
  }

  return count;
}

std::string Logger::DumpTracePoints(const std::string& pattern)
{
  std::lock_guard guard(TracePointLock);

  std::string response;
  for (TracePoint* point = TracePoints; point; point = point->Next)
  {
    if (!Detail::TracePointMatch(*point, pattern))
      continue;

    response += point->Enabled ? "on  " : "off ";
    response += point->Module ? point->Module : "";
    response += ":";
    response += point->Method ? point->Method : "";
    response += ":";
    response += std::to_string(point->Line);
    response += " hits=";
    response += std::to_string(point->GetCounter());
    response += "\n";
  }

  if (response.empty())
    response = "none\n";

  return response;
}

bool Logger::CommandTrace(StringArray& arr, std::string& response)
{
  if (arr.size() == 1 || arr[1] == "list" || arr[1] == "stat" || arr[1] == "stats")
  {
    std::string pattern;
    if (arr.size() >= 3)
      pattern = arr[2];

    response = Instance->DumpTracePoints(pattern);
    return true;
  }

  if (arr[1] == "enable" || arr[1] == "disable")
  {
    if (arr.size() < 3)
    {
      response = "error: missing trace point pattern";
      return true;
    }

    bool enabled = arr[1] == "enable";
    size_t count = Instance->SetTracePointsEnabled(arr[2], enabled);
    response = "ok: ";
    response += std::to_string(count);
    response += enabled ? " enabled" : " disabled";
    return true;
  }

  if (arr[1] == "reset")
  {
    std::string pattern;
    if (arr.size() >= 3)
      pattern = arr[2];

    size_t count = Instance->ResetTracePointCounters(pattern);
    response = "ok: ";
    response += std::to_string(count);
    response += " reset";
    return true;
  }

  response = "error: unknown trace command: " + arr[1];
  return true;
}
