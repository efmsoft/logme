#include <Common/TestBackend.h>

#include <string>

#include <gtest/gtest.h>

#include <Logme/Logme.h>

std::shared_ptr<TestBackend> Be;

static Logme::ChannelPtr MakeChannel(const char* name, bool active)
{
  auto ch = Logme::Instance->CreateChannel(Logme::ID{name});
  ch->RemoveBackends();
  ch->SetEnabled(active);
  ch->AddBackend(Be);

  Logme::OutputFlags flags;
  flags.Value = 0;
  ch->SetFlags(flags);

  return ch;
}

// Neither of these overrides LoggerCondition() -- both must fall through to
// the global default (Logme::Instance->Condition() && GetThreadLogCondition()).
struct PlainComponent
{
  void LogSomething(const Logme::ChannelPtr& ch)
  {
    LogmeI(ch, "from PlainComponent");
  }
};

// Declares its own LoggerCondition() member, exactly like
// tests/LoggerConditionOverride's FlaggedComponent: ordinary C++ member
// lookup hides the global default (and therefore the thread condition
// baked into it) for every log macro invoked from this class's own
// methods. Proves a class with per-object state to cache its own answer
// on is never affected by LogmeThreadCondition() -- it already fully
// replaced the global default.
struct FlaggedComponent
{
  bool Enabled = true;

  bool LoggerCondition() const
  {
    return Enabled;
  }

  void LogSomething(const Logme::ChannelPtr& ch)
  {
    LogmeI(ch, "from FlaggedComponent");
  }
};

TEST(ThreadLogCondition, FalseSuppressesPlainComponentLogging)
{
  auto ch = MakeChannel("thread_log_condition_suppress", true);
  Be->Clear();

  {
    LogmeThreadCondition(false);

    PlainComponent c;
    c.LogSomething(ch);
  }

  EXPECT_TRUE(Be->History.empty());
}

TEST(ThreadLogCondition, TrueAllowsPlainComponentLogging)
{
  auto ch = MakeChannel("thread_log_condition_allow", true);
  Be->Clear();

  {
    LogmeThreadCondition(true);

    PlainComponent c;
    c.LogSomething(ch);
  }

  ASSERT_EQ(Be->History.size(), 1u);
  EXPECT_NE(Be->History[0].find("from PlainComponent"), std::string::npos);
}

TEST(ThreadLogCondition, RestoresPreviousStateOnScopeExit)
{
  auto ch = MakeChannel("thread_log_condition_restore", true);

  EXPECT_FALSE(LogmeThreadConditionDefined());

  {
    LogmeThreadCondition(false);
    EXPECT_TRUE(LogmeThreadConditionDefined());

    {
      LogmeThreadCondition(true);
      EXPECT_TRUE(LogmeThreadConditionDefined());

      Be->Clear();
      PlainComponent c;
      c.LogSomething(ch);
      ASSERT_EQ(Be->History.size(), 1u);
    }

    // Back to the outer false scope, not cleared entirely.
    Be->Clear();
    PlainComponent c;
    c.LogSomething(ch);
    EXPECT_TRUE(Be->History.empty());
  }

  EXPECT_FALSE(LogmeThreadConditionDefined());

  Be->Clear();
  PlainComponent c;
  c.LogSomething(ch);
  ASSERT_EQ(Be->History.size(), 1u);
}

TEST(ThreadLogCondition, DoesNotAffectClassWithItsOwnLoggerConditionOverride)
{
  auto ch = MakeChannel("thread_log_condition_override_unaffected", true);
  Be->Clear();

  {
    LogmeThreadCondition(false);

    FlaggedComponent c;
    c.Enabled = true;
    c.LogSomething(ch);
  }

  // FlaggedComponent's own LoggerCondition() member hides the global
  // default entirely -- the thread condition baked into that global
  // default is never even consulted.
  ASSERT_EQ(Be->History.size(), 1u);
  EXPECT_NE(Be->History[0].find("from FlaggedComponent"), std::string::npos);
}

// The motivating combination: code that reports a fixed subsystem for a
// whole stretch of work (LogmeThreadSubsystem) checks once, before that
// stretch starts, whether the subsystem is blocked -- via
// Logger::IsSubsystemBlocked(), the same list DoLog() would otherwise
// consult on every individual call -- and short-circuits the whole
// stretch with LogmeThreadCondition() instead of paying for channel
// resolution and argument preparation on each call only to be dropped at
// that last-resort check.
TEST(ThreadLogCondition, IsSubsystemBlockedDrivesThreadCondition)
{
  auto ch = MakeChannel("thread_log_condition_subsystem_blocked", true);
  Logme::SID dsl = Logme::SID::Build("thread_log_condition_test_dsl");

  Logme::Instance->AddBlockedSubsystem(dsl);

  {
    LogmeThreadCondition(!Logme::Instance->IsSubsystemBlocked(dsl));

    Be->Clear();
    PlainComponent c;
    c.LogSomething(ch);
    EXPECT_TRUE(Be->History.empty());
  }

  Logme::Instance->RemoveBlockedSubsystem(dsl);

  {
    LogmeThreadCondition(!Logme::Instance->IsSubsystemBlocked(dsl));

    Be->Clear();
    PlainComponent c;
    c.LogSomething(ch);
    ASSERT_EQ(Be->History.size(), 1u);
  }
}

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
