#include "Logme/ReentryGuard.h"

using namespace Logme;

DisplayReentryGuard::State::State()
  : Depth(0)
  , Overflow(false)
  , SmallStack{}
{
}

DisplayReentryGuard::DisplayReentryGuard(const Channel *channel)
  : ChannelValue(channel)
  , EnteredValue(false)
{
  State &state = GetState();

  if (!state.Overflow)
  {
    int depth = state.Depth;

    if (depth > 0 && state.SmallStack[depth - 1] == channel)
    {
      return;
    }

    for (int i = 0; i < depth; ++i)
    {
      if (state.SmallStack[i] == channel)
      {
        return;
      }
    }

    if (depth < SMALL_CAPACITY)
    {
      state.SmallStack[depth] = channel;
      state.Depth = depth + 1;
      EnteredValue = true;
      return;
    }

    PromoteToOverflow(state);
  }

  auto &stack = state.LargeStack;

  for (const Channel *c : stack)
  {
    if (c == channel)
    {
      return;
    }
  }

  stack.push_back(channel);
  EnteredValue = true;
}

DisplayReentryGuard::~DisplayReentryGuard()
{
  if (!EnteredValue)
  {
    return;
  }

  State &state = GetState();

  if (!state.Overflow)
  {
    --state.Depth;
    return;
  }

  state.LargeStack.pop_back();
}

bool DisplayReentryGuard::IsActive() const
{
  return EnteredValue;
}

DisplayReentryGuard::State &DisplayReentryGuard::GetState()
{
  thread_local State state;
  return state;
}

void DisplayReentryGuard::PromoteToOverflow(State &state)
{
  state.Overflow = true;

  auto &stack = state.LargeStack;
  stack.reserve(SMALL_CAPACITY * 2);

  for (int i = 0; i < state.Depth; ++i)
  {
    stack.push_back(state.SmallStack[i]);
  }
}