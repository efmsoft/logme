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

  Shortener = nullptr;
}

static uint64_t MakeKey(const char* func, int line)
{
  uint64_t h = (uint64_t)(uintptr_t)func;
  h ^= (uint32_t)line + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
  return h;
}

OneTimeOverrideGenerator::OneTimeOverrideGenerator()
{
}

Override& OneTimeOverrideGenerator::GetOneTimeOverride(
  const char* func
  , int line
)
{
  auto key = MakeKey(func, line);
  auto it = Overrides.find(key);

  if (it == Overrides.end())
    it = Overrides.emplace(key, Override(1)).first;
  
  return it->second;
}

EveryTimeOverrideGenerator::EveryTimeOverrideGenerator()
{
}

Override& EveryTimeOverrideGenerator::GetEveryOverride(
  const char* func
  , int line
  , uint64_t ms
)
{
  auto key = MakeKey(func, line);
  auto it = Overrides.find(key);

  if (it == Overrides.end())
    it = Overrides.emplace(key, Override(-1, ms)).first;
  
  return it->second;
}
