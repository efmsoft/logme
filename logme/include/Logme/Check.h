#pragma once

#include <atomic>
#include <cstdint>
#include <sstream>
#include <string>
#include <utility>

#include <Logme/Types.h>

namespace Logme
{
  namespace Detail
  {
    struct CheckResult
    {
      bool Ok;
      std::string Message;

      CheckResult(bool ok, std::string message)
        : Ok(ok)
        , Message(std::move(message))
      {
      }
    };

    LOGMELNK std::string CaptureSystemErrorText();

    template<typename Left, typename Right, typename Predicate>
    inline CheckResult MakeCheckResult(
      Left&& left
      , Right&& right
      , const char* leftExpression
      , const char* rightExpression
      , const char* checkName
      , const char* operation
      , Predicate&& predicate
    )
    {
      if (predicate(left, right))
        return CheckResult(true, std::string());

      std::ostringstream message;
      message
        << checkName
        << " failed: "
        << leftExpression
        << " "
        << operation
        << " "
        << rightExpression
        << " ("
        << left
        << " vs "
        << right
        << ")";

      return CheckResult(false, message.str());
    }

    class FirstNState
    {
      std::atomic<std::uint64_t> Counter;

    public:
      FirstNState()
        : Counter(0)
      {
      }

      bool ShouldLog(std::uint64_t limit)
      {
        if (limit == 0)
          return false;

        return Counter.fetch_add(1, std::memory_order_relaxed) < limit;
      }
    };

    class EveryNState
    {
      std::atomic<std::uint64_t> Counter;

    public:
      EveryNState()
        : Counter(0)
      {
      }

      bool ShouldLog(std::uint64_t interval)
      {
        if (interval == 0)
          return false;

        const std::uint64_t value = Counter.fetch_add(1, std::memory_order_relaxed) + 1;
        return (value % interval) == 0;
      }
    };
  }
}
