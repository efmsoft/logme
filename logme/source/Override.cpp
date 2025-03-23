#include <Logme/Override.h>

using namespace Logme;

Override::Override(int reps, uint64_t noMoreThanOnceEveryXMillisec)
{
  Add.Value = 0;
  Remove.Value = 0;

  MaxRepetitions = reps;
  Repetitions = 0;
  
  MaxFrequency = noMoreThanOnceEveryXMillisec;
  LastTime = 0;
}

OneTimeMessage::OneTimeMessage()
  : Override(1)
{
}

OneTimeOverrideGenerator::OneTimeOverrideGenerator()
{ }

Override& OneTimeOverrideGenerator::GetOneTimeOverride(
  const char* func
  , int line
)
{
  std::string key(func);
  key += std::to_string(line);

  if (Overrides.find(key) == Overrides.end())
    Overrides[key] = OneTimeMessage();
  
  return Overrides[key];
}
