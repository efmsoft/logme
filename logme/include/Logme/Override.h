#pragma once

#include <memory>

#include <Logme/OutputFlags.h>
#include <Logme/Types.h>

namespace Logme
{
  struct ShortenerPair
  {
    const char* SerachFor;
    const char* ReplaceOn;
  };

  struct Override
  {
    OutputFlags Add;
    OutputFlags Remove;

    int MaxRepetitions;
    int Repetitions;

    uint64_t MaxFrequency;
    uint64_t LastTime;

    ShortenerPair* Shortener;

    LOGMELNK Override(
      int reps = -1
      , uint64_t noMoreThanOnceEveryXMillisec = 0
    );
  };

  typedef std::shared_ptr<Override> OverridePtr;

  struct OneTimeMessage : public Override
  {
    LOGMELNK OneTimeMessage();
  };

  struct OneTimeOverrideGenerator
  {
    std::map<std::string, Override> Overrides;

    LOGMELNK OneTimeOverrideGenerator();

    LOGMELNK virtual Override& GetOneTimeOverride(
      const char* func
      , int line
    );
  };
}

// This macro can ve used to get a one-time message override unique for this object
#define LOGME_ONCE4THIS GetOneTimeOverride(__func__, __LINE__)
