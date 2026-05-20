#include <Logme/Logme.h>
#include <Logme/Backend/CallbackBackend.h>
#include <Logme/Channel.h>

#include <cstdio>
#include <memory>

namespace
{
  struct CallbackState
  {
    size_t Count;
  };

  void StoreRecord(
    Logme::Context& context
    , const Logme::ChannelPtr& owner
    , void* userData
  )
  {
    auto state = static_cast<CallbackState*>(userData);

    int nc = 0;
    const char* text = context.Apply(owner, owner->GetFlags(), nc);

    ++state->Count;
    std::printf(
      "callback[%zu]: %.*s\n"
      , state->Count
      , nc
      , text
    );
  }
}

int main()
{
  Logme::ID id{"callback"};
  auto channel = Logme::Instance->CreateChannel(id);

  CallbackState state{};
  auto backend = std::make_shared<Logme::CallbackBackend>(
    channel
    , StoreRecord
    , &state
  );

  channel->AddBackend(backend);

  LogmeI(channel, "first callback record: %d", 1);
  LogmeW(channel) << "second callback record";

  std::printf("records captured by callback: %zu\n", state.Count);

  return 0;
}
