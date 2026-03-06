#pragma once

#include <vector>

namespace Logme
{
  class Channel;

  class DisplayReentryGuard
  {
    bool EnteredValue;

  public:
    explicit DisplayReentryGuard(const Channel *channel);
    ~DisplayReentryGuard();

    bool IsActive() const;

  private:
    static constexpr int SMALL_CAPACITY = 8;

    struct State
    {
      State();

      int Depth;
      bool Overflow;
      const Channel* SmallStack[SMALL_CAPACITY];
      std::vector<const Channel*> LargeStack;
    };

    static State &GetState();
    static void PromoteToOverflow(State &state);
  };
}