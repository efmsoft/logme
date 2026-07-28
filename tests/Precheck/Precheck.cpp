#include <Common/TestBackend.h>

#include <atomic>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <Logme/Logme.h>

std::shared_ptr<TestBackend> Be;

static int ArgCounter;
static int CodeCounter;

static const char* CountedText()
{
  ArgCounter++;
  return "counted";
}

static int CountedValue()
{
  CodeCounter++;
  return 77;
}

static Logme::ChannelPtr MakeChannel(const char* name, bool active)
{
  auto ch = Logme::Instance->CreateChannel(Logme::ID{name});
  ch->RemoveBackends();
  ch->SetEnabled(active);

  Logme::OutputFlags flags;
  flags.Value = 0;
  ch->SetFlags(flags);

  return ch;
}

TEST(Precheck, ChannelPtrSkipsArgumentEvaluationWhenInactive)
{
  auto ch = MakeChannel("precheck_inactive", false);

  ArgCounter = 0;
  Be->Clear();

  LogmeI(ch, "%s", CountedText());

  EXPECT_EQ(ArgCounter, 0);
  EXPECT_TRUE(Be->History.empty());
}

TEST(Precheck, ChannelPtrEvaluatesArgumentWhenActive)
{
  auto ch = MakeChannel("precheck_active", true);
  ch->AddBackend(Be);

  ArgCounter = 0;
  Be->Clear();

  LogmeI(ch, "%s", CountedText());

  EXPECT_EQ(ArgCounter, 1);
  ASSERT_EQ(Be->History.size(), 1u);
  EXPECT_NE(Be->History[0].find("counted"), std::string::npos);
}

TEST(Precheck, DoWithIdSkipsPreparationCodeWhenInactive)
{
  auto ch = MakeChannel("precheck_do_inactive", false);

  int value = 0;
  CodeCounter = 0;
  Be->Clear();

  LogmeI_Do(ch->GetID(), value = CountedValue(), "value=%d", value);

  EXPECT_EQ(CodeCounter, 0);
  EXPECT_EQ(value, 0);
  EXPECT_TRUE(Be->History.empty());
}

TEST(Precheck, DoWithIdRunsPreparationCodeWhenActive)
{
  auto ch = MakeChannel("precheck_do_active", true);
  ch->AddBackend(Be);

  int value = 0;
  CodeCounter = 0;
  Be->Clear();

  LogmeI_Do(ch->GetID(), value = CountedValue(), "value=%d", value);

  EXPECT_EQ(CodeCounter, 1);
  EXPECT_EQ(value, 77);
  ASSERT_EQ(Be->History.size(), 1u);
  EXPECT_NE(Be->History[0].find("value=77"), std::string::npos);
}


TEST(Precheck, NamedSubsystemCanRelaxChannelLevel)
{
  auto ch = MakeChannel("precheck_subsystem_relax", true);
  ch->AddBackend(Be);
  ch->SetFilterLevel(Logme::LEVEL_INFO);

  const Logme::SID sid = Logme::SID::Build("DSL");
  Logme::Instance->SetSubsystemLevel(sid, Logme::LEVEL_DEBUG);

  ArgCounter = 0;
  Be->Clear();

  {
    const Logme::SID SUBSID = sid;
    LogmeD(ch, "%s", CountedText());
  }

  EXPECT_EQ(ArgCounter, 1);
  ASSERT_EQ(Be->History.size(), 1u);
  EXPECT_NE(Be->History[0].find("counted"), std::string::npos);

  Logme::Instance->ClearSubsystemLevels();
}

TEST(Precheck, NamedSubsystemCanTightenChannelLevel)
{
  auto ch = MakeChannel("precheck_subsystem_tighten", true);
  ch->AddBackend(Be);
  ch->SetFilterLevel(Logme::LEVEL_INFO);

  const Logme::SID sid = Logme::SID::Build("CLOUD");
  Logme::Instance->SetSubsystemLevel(sid, Logme::LEVEL_WARN);

  ArgCounter = 0;
  Be->Clear();

  {
    const Logme::SID SUBSID = sid;
    LogmeI(ch, "%s", CountedText());
  }

  EXPECT_EQ(ArgCounter, 0);
  EXPECT_TRUE(Be->History.empty());

  Logme::Instance->ClearSubsystemLevels();
}

TEST(Precheck, ExplicitSubsystemOverridesLocalSubsystemDuringPrecheck)
{
  auto ch = MakeChannel("precheck_explicit_subsystem", true);
  ch->AddBackend(Be);
  ch->SetFilterLevel(Logme::LEVEL_INFO);

  const Logme::SID dsl = Logme::SID::Build("DSL");
  const Logme::SID cloud = Logme::SID::Build("CLOUD");
  Logme::Instance->SetSubsystemLevel(dsl, Logme::LEVEL_DEBUG);
  Logme::Instance->SetSubsystemLevel(cloud, Logme::LEVEL_WARN);

  ArgCounter = 0;
  Be->Clear();

  {
    const Logme::SID SUBSID = dsl;
    LogmeI(ch, cloud, "%s", CountedText());
  }

  EXPECT_EQ(ArgCounter, 0);
  EXPECT_TRUE(Be->History.empty());

  ArgCounter = 0;
  Be->Clear();

  {
    const Logme::SID SUBSID = cloud;
    LogmeD(ch, dsl, "%s", CountedText());
  }

  EXPECT_EQ(ArgCounter, 1);
  ASSERT_EQ(Be->History.size(), 1u);
  EXPECT_NE(Be->History[0].find("counted"), std::string::npos);

  Logme::Instance->ClearSubsystemLevels();
}

TEST(Precheck, ExplicitSubsystemIsDetectedAfterOverrideAndChannel)
{
  auto ch = MakeChannel("precheck_override_channel_subsystem", true);
  ch->AddBackend(Be);
  ch->SetFilterLevel(Logme::LEVEL_INFO);

  const Logme::SID cloud = Logme::SID::Build("CLOUD");
  Logme::Instance->SetSubsystemLevel(cloud, Logme::LEVEL_WARN);

  Logme::Override ovr;
  ArgCounter = 0;
  Be->Clear();

  LogmeI(ovr, ch, cloud, "%s", CountedText());

  EXPECT_EQ(ArgCounter, 0);
  EXPECT_TRUE(Be->History.empty());

  Logme::Instance->ClearSubsystemLevels();
}

TEST(Precheck, ThreadSubsystemDefersChannelLevelCheck)
{
  auto ch = MakeChannel("precheck_thread_subsystem", true);
  ch->AddBackend(Be);
  ch->SetFilterLevel(Logme::LEVEL_INFO);

  const Logme::SID sid = Logme::SID::Build("DSL");
  Logme::Instance->SetSubsystemLevel(sid, Logme::LEVEL_DEBUG);
  Logme::Instance->SetThreadSubsystem(&sid);

  ArgCounter = 0;
  Be->Clear();

  LogmeD(ch, "%s", CountedText());

  EXPECT_EQ(ArgCounter, 1);
  ASSERT_EQ(Be->History.size(), 1u);

  Logme::Instance->SetThreadSubsystem(&SUBSID);
  EXPECT_FALSE(Logme::Instance->IsSubsystemDefinedForCurrentThread());

  ArgCounter = 0;
  Be->Clear();

  LogmeD(ch, "%s", CountedText());

  EXPECT_EQ(ArgCounter, 0);
  EXPECT_TRUE(Be->History.empty());

  Logme::Instance->ClearSubsystemLevels();
}

TEST(Precheck, SubsystemPriorityIsExplicitThenThreadThenLocal)
{
  auto ch = MakeChannel("precheck_subsystem_priority", true);
  ch->AddBackend(Be);
  ch->SetFilterLevel(Logme::LEVEL_INFO);

  const Logme::SID dsl = Logme::SID::Build("DSL");
  const Logme::SID cloud = Logme::SID::Build("CLOUD");
  Logme::Instance->SetSubsystemLevel(dsl, Logme::LEVEL_DEBUG);
  Logme::Instance->SetSubsystemLevel(cloud, Logme::LEVEL_WARN);

  Logme::Instance->SetThreadSubsystem(&cloud);

  ArgCounter = 0;
  Be->Clear();

  {
    const Logme::SID SUBSID = dsl;
    LogmeI(ch, "%s", CountedText());
  }

  EXPECT_EQ(ArgCounter, 1);
  EXPECT_TRUE(Be->History.empty());

  ArgCounter = 0;
  Be->Clear();

  {
    const Logme::SID SUBSID = cloud;
    LogmeD(ch, dsl, "%s", CountedText());
  }

  EXPECT_EQ(ArgCounter, 1);
  ASSERT_EQ(Be->History.size(), 1u);
  EXPECT_NE(Be->History[0].find("counted"), std::string::npos);

  Logme::Instance->SetThreadSubsystem(&SUBSID);
  Logme::Instance->ClearSubsystemLevels();
}

TEST(Precheck, SubsystemLevelApiSupportsReplaceAndRemove)
{
  const Logme::SID sid = Logme::SID::Build("DSL");
  Logme::Level level = Logme::LEVEL_CRITICAL;

  Logme::Instance->ClearSubsystemLevels();
  EXPECT_FALSE(Logme::Instance->HasSubsystemLevelOverrides());
  EXPECT_FALSE(Logme::Instance->GetSubsystemLevel(sid, level));

  Logme::Instance->SetSubsystemLevel(sid, Logme::LEVEL_DEBUG);
  EXPECT_TRUE(Logme::Instance->HasSubsystemLevelOverrides());
  ASSERT_TRUE(Logme::Instance->GetSubsystemLevel(sid, level));
  EXPECT_EQ(level, Logme::LEVEL_DEBUG);

  Logme::Instance->SetSubsystemLevel(sid, Logme::LEVEL_WARN);
  ASSERT_TRUE(Logme::Instance->GetSubsystemLevel(sid, level));
  EXPECT_EQ(level, Logme::LEVEL_WARN);

  Logme::Instance->RemoveSubsystemLevel(sid);
  EXPECT_FALSE(Logme::Instance->HasSubsystemLevelOverrides());
  EXPECT_FALSE(Logme::Instance->GetSubsystemLevel(sid, level));
}

TEST(Precheck, SubsystemLevelSnapshotsSupportConcurrentUpdates)
{
  Logme::Logger logger;
  const Logme::SID dsl = Logme::SID::Build("DSL");
  const Logme::SID cloud = Logme::SID::Build("CLOUD");
  std::atomic<bool> stop{false};
  std::atomic<int> failures{0};

  std::vector<std::thread> readers;
  for (int i = 0; i < 4; ++i)
  {
    readers.emplace_back([&]()
    {
      while (!stop.load(std::memory_order_relaxed))
      {
        Logme::Level level = Logme::LEVEL_CRITICAL;
        if (
             logger.GetSubsystemLevel(dsl, level)
          && level != Logme::LEVEL_DEBUG
          && level != Logme::LEVEL_WARN
        )
        {
          failures.fetch_add(1, std::memory_order_relaxed);
        }

        logger.GetSubsystemLevel(cloud, level);
        logger.HasSubsystemLevelOverrides();
      }
    });
  }

  for (int i = 0; i < 1000; ++i)
  {
    logger.SetSubsystemLevel(
      dsl
      , (i & 1) ? Logme::LEVEL_DEBUG : Logme::LEVEL_WARN
    );
    logger.SetSubsystemLevel(cloud, Logme::LEVEL_INFO);
    logger.RemoveSubsystemLevel(cloud);
    if ((i % 17) == 0)
      logger.ClearSubsystemLevels();
  }

  stop.store(true, std::memory_order_relaxed);
  for (auto& reader : readers)
    reader.join();

  EXPECT_EQ(failures.load(std::memory_order_relaxed), 0);
}

#ifndef LOGME_DISABLE_STD_FORMAT
TEST(Precheck, StdFormatDoWithIdSkipsPreparationCodeWhenInactive)
{
  auto ch = MakeChannel("precheck_fmt_do_inactive", false);

  int value = 0;
  CodeCounter = 0;
  Be->Clear();

  fLogmeI_Do(ch->GetID(), value = CountedValue(), "value={}", value);

  EXPECT_EQ(CodeCounter, 0);
  EXPECT_EQ(value, 0);
  EXPECT_TRUE(Be->History.empty());
}
#endif

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  auto ch = Logme::Instance->GetExistingChannel(CH);
  ch->RemoveBackends();
  Be = std::make_shared<TestBackend>(ch);

  Logme::OutputFlags flags;
  flags.Value = 0;
  ch->SetFlags(flags);

  return RUN_ALL_TESTS();
}
