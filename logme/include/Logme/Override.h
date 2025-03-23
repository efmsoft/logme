#pragma once

#include <Logme/OutputFlags.h>
#include <Logme/Types.h>

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
      int reps = -1
      , uint64_t noMoreThanOnceEveryXMillisec = 0
    );
  };

  struct OneTimeMessage : public Override
  {
    LOGMELNK OneTimeMessage();
  };

  struct OneTimeOverrideGenerator
  {
    std::map<std::string, Override> Overrides;

    LOGMELNK OneTimeOverrideGenerator();

    LOGMELNK Override& GetOneTimeOverride(
      const char* func
      , int line
    );
  };
}

// This macro can ve used to get a one-time message override unique for this object
#define LOGME_ONCE4THIS GetOneTimeOverride(__func__, __LINE__)
