#include <Logme/Logme.h>

static void SameText()
{
  for (int i = 0; i < 4; i++)
  {
    LogmeW_Collapse(3, "network error: connection refused");
  }
}

static void IgnoreCorrelationId()
{
  for (int i = 0; i < 3; i++)
  {
    LogmeE_CollapseIgnore(
      "correlation-id=[0-9]+"
      , 2
      , "request failed: correlation-id=%d, network error: connection refused"
      , i
    );
  }
}

int main()
{
  SameText();
  IgnoreCorrelationId();

  return 0;
}
