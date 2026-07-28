#include <Common/TestBackend.h>

#include <atomic>
#include <string>
#include <thread>
#include <utility>
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

#ifndef LOGME_DISABLE_STD_FORMAT
static std::string BuildForwardedFormat(const std::string& format)
{
  return format;
}

template<typename... Args>
static void LogForwardedError(
  const std::string& format
  , Args&&... args
)
{
  fLogmeE(
    BuildForwardedFormat(format).c_str()
    , std::forward<Args>(args)...
    , "suffix"
  );
}

template<typename... Args>
static void LogForwardedError(
  const Logme::ID& id
  , const std::string& format
  , Args&&... args
)
{
  fLogmeE(
    id
    , BuildForwardedFormat(format).c_str()
    , std::forward<Args>(args)...
    , "suffix"
  );
}


template<typename... Args>
static void LogForwardedStdAfterChannel(
  const Logme::ChannelPtr& ch
  , Args&&... args
)
{
  fLogmeI(ch, std::forward<Args>(args)...);
}

template<typename... Args>
static void LogForwardedStdAfterFormat(
  const Logme::ChannelPtr& ch
  , const Logme::SID& localSubsystem
  , const char* format
  , Args&&... args
)
{
  const Logme::SID SUBSID = localSubsystem;

  fLogmeI(
    ch
    , format
    , std::forward<Args>(args)...
    , CountedText()
  );
}

template<typename... Args>
static void LogForwardedLegacyAfterChannel(
  const Logme::ChannelPtr& ch
  , Args&&... args
)
{
  LogmeI(ch, std::forward<Args>(args)...);
}

template<typename... Args>
static void LogForwardedAfterOverrideAndChannel(
  Logme::Override& ovr
  , const Logme::ChannelPtr& ch
  , Args&&... args
)
{
  fLogmeI(ovr, ch, std::forward<Args>(args)...);
}

template<typename... Args>
static void LogForwardedAfterOverride(
  Logme::Override& ovr
  , Args&&... args
)
{
  fLogmeI(ovr, std::forward<Args>(args)...);
}
#endif

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

TEST(Precheck, NamedSubsystemTighteningSkipsArgumentEvaluation)
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

TEST(Precheck, ExplicitSubsystemOverridesLocalSubsystemDuringDisplay)
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

TEST(Precheck, ExplicitEmptySubsystemUsesChannelLevelWithoutFallback)
{
  auto ch = MakeChannel("precheck_explicit_empty_subsystem", true);
  ch->AddBackend(Be);
  ch->SetFilterLevel(Logme::LEVEL_INFO);

  const Logme::SID local = Logme::SID::Build("LOCAL");
  const Logme::SID thread = Logme::SID::Build("THREAD");
  const Logme::SID empty{0};
  Logme::Instance->SetSubsystemLevel(local, Logme::LEVEL_WARN);
  Logme::Instance->SetSubsystemLevel(thread, Logme::LEVEL_WARN);
  Logme::Instance->SetThreadSubsystem(&thread);

  ArgCounter = 0;
  Be->Clear();

  {
    const Logme::SID SUBSID = local;
    LogmeI(ch, empty, "%s", CountedText());
  }

  EXPECT_EQ(ArgCounter, 1);
  ASSERT_EQ(Be->History.size(), 1u);
  EXPECT_NE(Be->History[0].find("counted"), std::string::npos);

  Logme::Instance->SetThreadSubsystem(&SUBSID);
  Logme::Instance->ClearSubsystemLevels();
}

TEST(Precheck, OverrideAndChannelUsesExplicitSubsystemInPrecheck)
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

  logger.SetSubsystemLevel(dsl, Logme::LEVEL_INFO);
  Logme::Level level = Logme::LEVEL_CRITICAL;
  ASSERT_TRUE(logger.GetSubsystemLevel(dsl, level));
  EXPECT_EQ(level, Logme::LEVEL_INFO);
}

#ifndef LOGME_DISABLE_STD_FORMAT
TEST(Precheck, StdFormatCompilesAllForwardedArgumentLayouts)
{
  auto ch = MakeChannel("precheck_forwarded_layouts", true);
  const Logme::SID sid = Logme::SID::Build("FORWARDED");
  Logme::Override ovr;

  // These calls intentionally live in a non-executed branch. Their purpose is
  // to instantiate every supported forwarded layout during the local Logme
  // test build, including the layouts used by nfs-common/Exception.h.
  if (false)
  {
    LogForwardedError("default value={} {}", 41);
    LogForwardedError(ch->GetID(), "id value={} {}", 42);

    LogForwardedStdAfterChannel(ch, "channel value={}", 43);
    LogForwardedStdAfterChannel(ch, sid, "channel sid value={}", 44);
    LogForwardedLegacyAfterChannel(ch, sid, "channel sid value=%d", 45);

    fLogmeI(ch->GetID(), sid, "id sid");
    fLogmeI(ch->GetID(), sid, "id sid value={}", 46);
    fLogmeI(ch, sid, "channel sid");
    fLogmeI(ch, sid, "channel sid values={} {}", 47, 48);

    LogForwardedAfterOverrideAndChannel(ovr, ch, "override channel value={}", 49);
    LogForwardedAfterOverrideAndChannel(ovr, ch, sid, "override channel sid value={}", 50);

    LogForwardedAfterOverride(ovr, ch, "override packed channel value={}", 51);
    LogForwardedAfterOverride(ovr, ch, sid, "override packed channel sid value={}", 52);
  }

  SUCCEED();
}

TEST(Precheck, StdFormatSupportsIdAndChannelSubsystemOverloads)
{
  auto ch = MakeChannel("precheck_std_format_sid_overloads", true);
  ch->AddBackend(Be);
  ch->SetFilterLevel(Logme::LEVEL_INFO);

  const Logme::SID sid = Logme::SID::Build("FORMAT");

  Be->Clear();

  fLogmeI(ch->GetID(), sid, "id sid value={}", 11);
  fLogmeI(ch, sid, "channel sid value={}", 12);

  ASSERT_EQ(Be->History.size(), 2u);
  EXPECT_NE(Be->History[0].find("id sid value=11"), std::string::npos);
  EXPECT_NE(Be->History[1].find("channel sid value=12"), std::string::npos);
}

TEST(Precheck, StdFormatExplicitSubsystemCanRelaxChannelLevel)
{
  auto ch = MakeChannel("precheck_std_format_explicit_relax", true);
  ch->AddBackend(Be);
  ch->SetFilterLevel(Logme::LEVEL_INFO);

  const Logme::SID sid = Logme::SID::Build("FORMAT_EXPLICIT_RELAX");
  Logme::Instance->SetSubsystemLevel(sid, Logme::LEVEL_DEBUG);

  Logme::Override ovr;
  ArgCounter = 0;
  Be->Clear();

  fLogmeD(ch, sid, "channel explicit={}", CountedText());
  fLogmeD(ovr, ch, sid, "override channel explicit={}", CountedText());

  EXPECT_EQ(ArgCounter, 2);
  ASSERT_EQ(Be->History.size(), 2u);
  EXPECT_NE(Be->History[0].find("channel explicit=counted"), std::string::npos);
  EXPECT_NE(Be->History[1].find("override channel explicit=counted"), std::string::npos);

  Logme::Instance->ClearSubsystemLevels();
}

TEST(Precheck, StdFormatThreadSubsystemCanRelaxChannelLevel)
{
  auto ch = MakeChannel("precheck_std_format_thread_relax", true);
  ch->AddBackend(Be);
  ch->SetFilterLevel(Logme::LEVEL_INFO);

  const Logme::SID sid = Logme::SID::Build("FORMAT_THREAD_RELAX");
  Logme::Instance->SetSubsystemLevel(sid, Logme::LEVEL_DEBUG);
  Logme::Instance->SetThreadSubsystem(&sid);

  Logme::Override ovr;
  ArgCounter = 0;
  Be->Clear();

  fLogmeD(ch, "channel thread={}", CountedText());
  fLogmeD(ovr, ch, "override channel thread={}", CountedText());
  fLogmeD(ch, ovr, "channel override thread={}", CountedText());

  EXPECT_EQ(ArgCounter, 3);
  ASSERT_EQ(Be->History.size(), 3u);
  EXPECT_NE(Be->History[0].find("channel thread=counted"), std::string::npos);
  EXPECT_NE(Be->History[1].find("override channel thread=counted"), std::string::npos);
  EXPECT_NE(Be->History[2].find("channel override thread=counted"), std::string::npos);

  Logme::Instance->SetThreadSubsystem(&SUBSID);
  Logme::Instance->ClearSubsystemLevels();
}

TEST(Precheck, StdFormatExplicitTighteningSkipsArgumentEvaluation)
{
  auto ch = MakeChannel("precheck_std_format_explicit_tighten", true);
  ch->AddBackend(Be);
  ch->SetFilterLevel(Logme::LEVEL_INFO);

  const Logme::SID sid = Logme::SID::Build("FORMAT_EXPLICIT_TIGHTEN");
  Logme::Instance->SetSubsystemLevel(sid, Logme::LEVEL_WARN);

  Logme::Override ovr;
  ArgCounter = 0;
  Be->Clear();

  fLogmeI(ch, sid, "channel explicit={}", CountedText());
  fLogmeI(ovr, ch, sid, "override channel explicit={}", CountedText());

  EXPECT_EQ(ArgCounter, 0);
  EXPECT_TRUE(Be->History.empty());

  Logme::Instance->ClearSubsystemLevels();
}

TEST(Precheck, ForwardedPackAfterFormatDoesNotAffectSubsystemPrecheck)
{
  auto ch = MakeChannel("precheck_forwarded_after_format", true);
  ch->AddBackend(Be);
  ch->SetFilterLevel(Logme::LEVEL_INFO);

  const Logme::SID local = Logme::SID::Build("LOCAL_TIGHT");
  Logme::Instance->SetSubsystemLevel(local, Logme::LEVEL_WARN);

  ArgCounter = 0;
  Be->Clear();

  LogForwardedStdAfterFormat(
    ch
    , local
    , "value={} suffix={}"
    , 41
  );

  EXPECT_EQ(ArgCounter, 0);
  EXPECT_TRUE(Be->History.empty());

  Logme::Instance->ClearSubsystemLevels();
}

TEST(Precheck, ForwardedPackStartingWithSubsystemDefersWithoutBreakingBuild)
{
  auto ch = MakeChannel("precheck_forwarded_sid_pack", true);
  ch->AddBackend(Be);
  ch->SetFilterLevel(Logme::LEVEL_INFO);

  const Logme::SID sid = Logme::SID::Build("PACKED_TIGHT");
  Logme::Instance->SetSubsystemLevel(sid, Logme::LEVEL_WARN);

  ArgCounter = 0;
  Be->Clear();

  LogForwardedLegacyAfterChannel(ch, sid, "value=%s", CountedText());

  // The SID value is inside a C++ pack expansion. Reading only that value
  // would also evaluate the remaining formatting arguments, so this layout is
  // deliberately deferred to Channel::Display.
  EXPECT_EQ(ArgCounter, 1);
  EXPECT_TRUE(Be->History.empty());

  Logme::Instance->ClearSubsystemLevels();
}
#endif

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
