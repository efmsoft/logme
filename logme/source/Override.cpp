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