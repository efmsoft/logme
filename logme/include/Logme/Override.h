#pragma once

#include <Logme/OutputFlags.h>
#include <Logme/Types.h>

#define LOGME_ONCE 1
#define LOGME_ALWAYS -1

namespace Logme
{
  struct Override
  {
    OutputFlags Add;
    OutputFlags Remove;

    int MaxRepetitions;
    int Repetitions;

    uint64_t MaxFrequency;
    uint64_t LastTime;

    LOGMELNK Override(
      int reps = LOGME_ALWAYS
      , uint64_t noMoreThanOnceEveryXMillisec = 0
    );
  };
}
