#include <cctype>
#include <cstdlib>
#include <string>

#include <Logme/Logger.h>

#include "../../LogStatisticsInternal.h"

using namespace Logme;

namespace
{
  const size_t DEFAULT_LIMIT = 20;
  const size_t MAX_LIMIT = 1000;

  bool ParseLimit(
    const std::string& text
    , size_t& limit
  )
  {
    if (text.empty())
      return false;

    for (char ch : text)
    {
      if (!std::isdigit(static_cast<unsigned char>(ch)))
        return false;
    }

    unsigned long long value = std::strtoull(text.c_str(), nullptr, 10);
    if (value == 0 || value > MAX_LIMIT)
      return false;

    limit = static_cast<size_t>(value);
    return true;
  }

  bool ParseReportOptions(
    const StringArray& arr
    , size_t begin
    , LogStatisticsSort& sort
    , size_t& limit
    , std::string& response
    , std::string* backendType = nullptr
  )
  {
    sort = LogStatisticsSort::MESSAGE_BYTES;
    limit = DEFAULT_LIMIT;

    for (size_t i = begin; i < arr.size(); ++i)
    {
      const std::string& option = arr[i];

      if (option == "--sort")
      {
        if (++i >= arr.size())
        {
          response = "error: missing logstat sort value";
          return false;
        }

        if (arr[i] == "records")
          sort = LogStatisticsSort::RECORDS;
        else if (arr[i] == "bytes" || arr[i] == "message-bytes")
          sort = LogStatisticsSort::MESSAGE_BYTES;
        else
        {
          response = "error: invalid logstat sort value: " + arr[i];
          return false;
        }

        continue;
      }

      if (option == "--limit")
      {
        if (++i >= arr.size())
        {
          response = "error: missing logstat limit";
          return false;
        }

        if (!ParseLimit(arr[i], limit))
        {
          response = "error: invalid logstat limit: " + arr[i];
          return false;
        }

        continue;
      }

      if (option == "--backend")
      {
        if (backendType == nullptr)
        {
          response = "error: --backend is not supported for this report";
          return false;
        }

        if (++i >= arr.size() || arr[i].empty())
        {
          response = "error: missing logstat backend type";
          return false;
        }

        *backendType = arr[i];
        continue;
      }

      if (option == "records")
      {
        sort = LogStatisticsSort::RECORDS;
        continue;
      }

      if (option == "bytes" || option == "message-bytes")
      {
        sort = LogStatisticsSort::MESSAGE_BYTES;
        continue;
      }

      size_t parsedLimit = 0;
      if (ParseLimit(option, parsedLimit))
      {
        limit = parsedLimit;
        continue;
      }

      response = "error: unknown logstat option: " + option;
      return false;
    }

    return true;
  }

  bool ParseFileReportOptions(
    const StringArray& arr
    , size_t begin
    , LogFileStatisticsSort& sort
    , size_t& limit
    , std::string& response
  )
  {
    sort = LogFileStatisticsSort::WRITTEN_BYTES;
    limit = DEFAULT_LIMIT;

    for (size_t i = begin; i < arr.size(); ++i)
    {
      const std::string& option = arr[i];

      if (option == "--sort")
      {
        if (++i >= arr.size())
        {
          response = "error: missing logstat file sort value";
          return false;
        }

        const std::string& value = arr[i];
        if (value == "bytes" || value == "written-bytes")
          sort = LogFileStatisticsSort::WRITTEN_BYTES;
        else if (value == "records" || value == "batches"
          || value == "write-batches")
        {
          sort = LogFileStatisticsSort::WRITE_BATCHES;
        }
        else if (value == "errors" || value == "write-errors")
          sort = LogFileStatisticsSort::WRITE_ERRORS;
        else if (value == "dropped" || value == "dropped-bytes")
          sort = LogFileStatisticsSort::DROPPED_BYTES;
        else
        {
          response = "error: invalid logstat file sort value: " + value;
          return false;
        }

        continue;
      }

      if (option == "--limit")
      {
        if (++i >= arr.size())
        {
          response = "error: missing logstat limit";
          return false;
        }

        if (!ParseLimit(arr[i], limit))
        {
          response = "error: invalid logstat limit: " + arr[i];
          return false;
        }

        continue;
      }

      if (option == "bytes" || option == "written-bytes")
      {
        sort = LogFileStatisticsSort::WRITTEN_BYTES;
        continue;
      }

      if (option == "records" || option == "batches"
        || option == "write-batches")
      {
        sort = LogFileStatisticsSort::WRITE_BATCHES;
        continue;
      }

      if (option == "errors" || option == "write-errors")
      {
        sort = LogFileStatisticsSort::WRITE_ERRORS;
        continue;
      }

      if (option == "dropped" || option == "dropped-bytes")
      {
        sort = LogFileStatisticsSort::DROPPED_BYTES;
        continue;
      }

      size_t parsedLimit = 0;
      if (ParseLimit(option, parsedLimit))
      {
        limit = parsedLimit;
        continue;
      }

      response = "error: unknown logstat file option: " + option;
      return false;
    }

    return true;
  }
}

bool Logger::CommandLogStatistics(
  StringArray& arr
  , std::string& response
)
{
  std::string operation = arr.size() >= 2 ? arr[1] : "status";

  if (operation == "start")
  {
    Instance->StartLogStatistics();
    response = "ok: log statistics started";
    return true;
  }

  if (operation == "stop")
  {
    Instance->StopLogStatistics();
    response = "ok: log statistics stopped";
    return true;
  }

  if (operation == "reset")
  {
    Instance->ResetLogStatistics();
    response = "ok: log statistics reset";
    return true;
  }

  if (operation == "status" || operation == "stat" || operation == "stats")
  {
    response = Instance->DumpLogStatisticsStatus();
    return true;
  }

  if (operation == "top" || operation == "sites")
  {
    LogStatisticsSort sort;
    size_t limit;
    if (!ParseReportOptions(arr, 2, sort, limit, response))
      return true;

    response = Instance->DumpLogStatisticsTop(sort, limit);
    return true;
  }

  if (operation == "channels")
  {
    LogStatisticsSort sort;
    size_t limit;
    if (!ParseReportOptions(arr, 2, sort, limit, response))
      return true;

    response = Instance->DumpLogStatisticsChannels(sort, limit);
    return true;
  }

  if (operation == "outputs" || operation == "output")
  {
    LogStatisticsSort sort;
    size_t limit;
    std::string backendType;
    if (!ParseReportOptions(arr, 2, sort, limit, response, &backendType))
      return true;

    response = Instance->DumpLogStatisticsOutputs(
      sort
      , limit
      , backendType
    );
    return true;
  }

  if (operation == "backends")
  {
    LogStatisticsSort sort;
    size_t limit;
    std::string backendType;
    if (!ParseReportOptions(arr, 2, sort, limit, response, &backendType))
      return true;

    response = Instance->DumpLogStatisticsBackends(
      sort
      , limit
      , backendType
    );
    return true;
  }

  if (operation == "files"
    || operation == "file"
    || operation == "file-backends")
  {
    LogFileStatisticsSort sort;
    size_t limit;
    if (!ParseFileReportOptions(arr, 2, sort, limit, response))
      return true;

    response = Instance->DumpLogStatisticsFileBackends(sort, limit);
    return true;
  }

  if (operation == "help")
  {
    response =
      "logstat start                              Start a new statistics session\n"
      "logstat stop                               Stop collection and preserve results\n"
      "logstat status                             Display current session status\n"
      "logstat reset                              Reset counters without changing active state\n"
      "logstat top [--sort bytes|records] [--limit count]\n"
      "                                           Display top source locations\n"
      "logstat channels [--sort bytes|records] [--limit count]\n"
      "                                           Display source channel totals\n"
      "logstat outputs [--sort bytes|records] [--limit count] [--backend type]\n"
      "                                           Display source sites by destination backend\n"
      "logstat backends [--sort bytes|records] [--limit count] [--backend type]\n"
      "                                           Display destination backend totals\n"
      "logstat files [--sort written-bytes|batches|errors|dropped-bytes] [--limit count]\n"
      "                                           Display asynchronous FileBackend runtime statistics\n";
    return true;
  }

  response = "error: unknown logstat command: " + operation;
  return true;
}
