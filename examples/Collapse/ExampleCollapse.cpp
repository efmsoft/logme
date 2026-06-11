#include <chrono>
#include <thread>

#include <Logme/Logme.h>

static void SameText()
{
  LogmeI("count-based collapse follows: first warning is printed, repeats wait for the counter limit");

  for (int i = 0; i < 4; i++)
  {
    LogmeW_Collapse(3, "network error: connection refused");
    LogmeI("retry loop is still active, attempt %d", i);
  }
  LogmeI("\n");
}

static void IgnoreCorrelationId()
{
  LogmeI("collapse-ignore follows: correlation-id changes, but it is ignored for repeat matching");

  for (int i = 0; i < 3; i++)
  {
    LogmeE_CollapseIgnore(
      "correlation-id=[0-9]+"
      , 2
      , "request failed: correlation-id=%d, network error: connection refused"
      , i
    );
  }
  LogmeI("\n");
}

static void LogBackendFailureEvery()
{
  LogmeW_CollapseEvery(
    100
    , "backend connection failed; retrying"
  );
}

static void SameTextEvery()
{
  LogmeI("time-based collapse follows: repeats are suppressed for 100 ms and counted");

  for (int i = 0; i < 5; i++)
  {
    LogBackendFailureEvery();
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(120));

  LogBackendFailureEvery();
  LogmeI("\n");
}

static void LogRequestFailureEvery(int requestId)
{
  LogmeW_CollapseIgnoreEvery(
    "request=[0-9]+"
    , 100
    , "request=%d backend connection failed; retrying"
    , requestId
  );
}

static void IgnoreChangingRequestEvery()
{
  LogmeI("time-based collapse-ignore follows: request id changes, but repeats are still collapsed");

  for (int i = 0; i < 4; i++)
  {
    LogRequestFailureEvery(i);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(120));

  LogRequestFailureEvery(4);
}

int main()
{
  LogmeI("collapse example: count-based collapse, ignored volatile fields, and time-based collapse\n");

  SameText();
  IgnoreCorrelationId();
  SameTextEvery();
  IgnoreChangingRequestEvery();

  return 0;
}
