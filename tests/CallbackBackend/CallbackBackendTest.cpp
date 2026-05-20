#include <gtest/gtest.h>

#include <Logme/Backend/CallbackBackend.h>
#include <Logme/Backend/Backend.h>
#include <Logme/Logme.h>

#include <string>

namespace
{
  const Logme::ID CallbackTestChannel{"callback-backend-test"};

  struct CallbackState
  {
    int Count = 0;
    Logme::ChannelPtr Owner;
    std::string Text;
  };

  void CollectCallback(
    Logme::Context& context
    , const Logme::ChannelPtr& owner
    , void* userData
  )
  {
    auto state = static_cast<CallbackState*>(userData);
    ASSERT_NE(state, nullptr);
    ASSERT_NE(owner, nullptr);

    int nc = 0;
    const char* buffer = context.Apply(
      owner
      , owner->GetFlags()
      , nc
    );

    state->Count++;
    state->Owner = owner;
    state->Text.assign(buffer, static_cast<size_t>(nc));
  }

  Logme::ChannelPtr CreateChannel()
  {
    Logme::OutputFlags flags;
    flags.Value = 0;
    flags.Eol = false;

    auto channel = Logme::Instance->CreateChannel(
      CallbackTestChannel
      , flags
      , Logme::LEVEL_DEBUG
    );
    channel->RemoveBackends();
    channel->SetFlags(flags);
    channel->SetFilterLevel(Logme::LEVEL_DEBUG);
    return channel;
  }
}

TEST(CallbackBackendTest, FactoryCreatesBackend)
{
  auto channel = CreateChannel();
  auto backend = Logme::Backend::Create(
    Logme::CallbackBackend::TYPE_ID
    , channel
  );

  ASSERT_NE(backend, nullptr);
  EXPECT_STREQ(backend->GetType(), Logme::CallbackBackend::TYPE_ID);
}

TEST(CallbackBackendTest, NullCallbackDoesNothing)
{
  auto channel = CreateChannel();
  auto backend = std::make_shared<Logme::CallbackBackend>(channel);
  channel->AddBackend(backend);

  EXPECT_EQ(backend->GetCallback(), nullptr);
  EXPECT_EQ(backend->GetUserData(), nullptr);
  EXPECT_EQ(backend->FormatDetails(), "Callback=null");

  LogmeI(CallbackTestChannel, "ignored");
}

TEST(CallbackBackendTest, InvokesCallbackWithOwnerUserDataAndFormattedRecord)
{
  auto channel = CreateChannel();
  CallbackState state;

  auto backend = std::make_shared<Logme::CallbackBackend>(
    channel
    , CollectCallback
    , &state
  );
  channel->AddBackend(backend);

  EXPECT_EQ(backend->GetCallback(), CollectCallback);
  EXPECT_EQ(backend->GetUserData(), &state);
  EXPECT_EQ(backend->FormatDetails(), "Callback=set");

  LogmeI(CallbackTestChannel, "callback value=%d", 42);

  EXPECT_EQ(state.Count, 1);
  EXPECT_EQ(state.Owner, channel);
  EXPECT_EQ(state.Text, "callback value=42");
}

TEST(CallbackBackendTest, SetCallbackReplacesCallbackAndUserData)
{
  auto channel = CreateChannel();
  CallbackState state;

  auto backend = std::make_shared<Logme::CallbackBackend>(channel);
  backend->SetCallback(CollectCallback, &state);
  channel->AddBackend(backend);

  LogmeW(CallbackTestChannel, "second path");

  EXPECT_EQ(state.Count, 1);
  EXPECT_EQ(state.Owner, channel);
  EXPECT_EQ(state.Text, "second path");
}

TEST(CallbackBackendTest, ApplyConfigAcceptsMatchingTypeOnly)
{
  auto channel = CreateChannel();
  auto backend = std::make_shared<Logme::CallbackBackend>(channel);

  auto goodConfig = std::make_shared<Logme::CallbackBackendConfig>();
  EXPECT_TRUE(backend->ApplyConfig(goodConfig));

  auto badConfig = std::make_shared<Logme::BackendConfig>("OtherBackend");
  EXPECT_FALSE(backend->ApplyConfig(badConfig));
}
