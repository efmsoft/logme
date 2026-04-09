#pragma once

#include <memory>
#include <unordered_map>

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

  struct OneTimeOverrideGenerator
  {
    std::unordered_map<uint64_t, Override> Overrides;

    LOGMELNK OneTimeOverrideGenerator();

    LOGMELNK virtual Override& GetOneTimeOverride(
      const char* func
      , int line
    );
  };

  struct EveryTimeOverrideGenerator
  {
    std::unordered_map<uint64_t, Override> Overrides;

    LOGMELNK EveryTimeOverrideGenerator();
    LOGMELNK virtual Override& GetEveryOverride(
      const char* func
      , int line
      , uint64_t ms
    );
  };

  struct OverrideGenerator 
    : public OneTimeOverrideGenerator
    , public EveryTimeOverrideGenerator
  {
  };
}

// This macro can be used to get a one-time message override unique for this object
#define LOGME_ONCE4THIS GetOneTimeOverride(__func__, __LINE__)

// This macro can be used to get a rate-limited message override unique for this object
#define LOGME_EVERY4THIS(ms) GetEveryOverride(__func__, __LINE__, ms)

#define LOGME_CALL_SCOPE \
  Logme::OverrideGenerator please_define_macro_LOGME_CALL_SCOPE

// This macro can be used to get a one-time message override unique for the current call (function invocation).
// Requires LOGME_CALL_SCOPE to be declared in the current scope.
#define LOGME_ONCE4CALL \
  please_define_macro_LOGME_CALL_SCOPE.GetOneTimeOverride(__func__, __LINE__)

#define LOGME_EVERY4CALL(ms) \
  please_define_macro_LOGME_CALL_SCOPE.GetEveryOverride(__func__, __LINE__, ms)
