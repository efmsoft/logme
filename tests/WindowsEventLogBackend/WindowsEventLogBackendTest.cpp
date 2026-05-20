#include <gtest/gtest.h>

#include <Logme/Backend/Backend.h>
#include <Logme/Backend/WindowsEventLogBackend.h>
#include <Logme/Logme.h>

#include <memory>
#include <string>

namespace
{
  const Logme::ID EventLogTestChannel{"windows-event-log-backend-test"};

  Logme::ChannelPtr CreateChannel()
  {
    Logme::OutputFlags flags;
    flags.Value = 0;
    flags.Eol = false;

    auto channel = Logme::Instance->CreateChannel(
      EventLogTestChannel
      , flags
      , Logme::LEVEL_DEBUG
    );
    channel->RemoveBackends();
    channel->SetFlags(flags);
    channel->SetFilterLevel(Logme::LEVEL_DEBUG);
    return channel;
  }
}

TEST(WindowsEventLogBackendTest, FactoryCreatesBackend)
{
  auto channel = CreateChannel();
  auto backend = Logme::Backend::Create(
    Logme::WindowsEventLogBackend::TYPE_ID
    , channel
  );

  ASSERT_NE(backend, nullptr);
  EXPECT_STREQ(backend->GetType(), Logme::WindowsEventLogBackend::TYPE_ID);
}

TEST(WindowsEventLogBackendTest, DefaultsAreUsable)
{
  auto channel = CreateChannel();
  auto backend = std::make_shared<Logme::WindowsEventLogBackend>(channel);

  EXPECT_TRUE(backend->IsAsyncSupported());
  EXPECT_FALSE(backend->GetSource().empty());
  EXPECT_EQ(backend->GetEventId(), 1000u);
  EXPECT_EQ(backend->GetCategory(), 0u);

  auto details = backend->FormatDetails();
  EXPECT_NE(details.find("SYNC"), std::string::npos);
  EXPECT_NE(details.find("Source="), std::string::npos);
  EXPECT_NE(details.find("EventId=1000"), std::string::npos);
  EXPECT_NE(details.find("Category=0"), std::string::npos);
}

TEST(WindowsEventLogBackendTest, ApplyConfigUpdatesPublicSettings)
{
  auto channel = CreateChannel();
  auto backend = std::make_shared<Logme::WindowsEventLogBackend>(channel);
  auto config = std::make_shared<Logme::WindowsEventLogBackendConfig>();

  config->Source = "LogmeTestSource";
  config->EventId = 1234;
  config->Category = 7;
  config->Async = true;

  EXPECT_TRUE(backend->ApplyConfig(config));
  EXPECT_TRUE(backend->GetAsync());
  EXPECT_EQ(backend->GetSource(), "LogmeTestSource");
  EXPECT_EQ(backend->GetEventId(), 1234u);
  EXPECT_EQ(backend->GetCategory(), 7u);

  auto details = backend->FormatDetails();
  EXPECT_NE(details.find("ASYNC"), std::string::npos);
  EXPECT_NE(details.find("Source=LogmeTestSource"), std::string::npos);
  EXPECT_NE(details.find("EventId=1234"), std::string::npos);
  EXPECT_NE(details.find("Category=7"), std::string::npos);
}

TEST(WindowsEventLogBackendTest, SyncDisplayAndFlushComplete)
{
  auto channel = CreateChannel();
  auto backend = std::make_shared<Logme::WindowsEventLogBackend>(channel);
  auto config = std::make_shared<Logme::WindowsEventLogBackendConfig>();
  config->Source = "LogmeTestSource";
  config->EventId = 1235;
  config->Category = 0;

  ASSERT_TRUE(backend->ApplyConfig(config));
  channel->AddBackend(backend);

  LogmeE(EventLogTestChannel, "sync event log backend test");
  backend->Flush();

  EXPECT_TRUE(backend->IsIdle());
}

TEST(WindowsEventLogBackendTest, AsyncDisplayFlushAndFreezeComplete)
{
  auto channel = CreateChannel();
  auto backend = std::make_shared<Logme::WindowsEventLogBackend>(channel);
  auto config = std::make_shared<Logme::WindowsEventLogBackendConfig>();
  config->Source = "LogmeTestSource";
  config->EventId = 1236;
  config->Category = 1;
  config->Async = true;

  ASSERT_TRUE(backend->ApplyConfig(config));
  channel->AddBackend(backend);

  LogmeW(EventLogTestChannel, "async event log backend test");
  backend->Flush();
  EXPECT_TRUE(backend->IsIdle() || backend->IsAsyncActive());

  backend->Freeze();
  EXPECT_TRUE(backend->IsIdle());
}
