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

// Every LogmeD/LogmeI/LogmeW/LogmeE/... macro calls the unqualified,
// global LoggerCondition() instead of Logme::Instance->Condition()
// directly (a qualified Logme::LoggerCondition() call could never be
// shadowed this way -- qualified lookup only ever considers the named
// namespace, never the caller's enclosing scope). A class that declares
// its own non-static member with this exact name hides the global default
// via ordinary C++ member lookup (checked before any enclosing scope is
// even considered) for any log macro invoked from that class's own
// methods -- the same mechanism this codebase already relies on to let
// CH/SUBSID resolve to a class member instead of the file-scope default
// (see Logme/ID.h, Logme/SID.h). No virtual dispatch, no change for
// callers that don't opt in.
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

// Does not override LoggerCondition() -- must keep using the process-wide
// default (Logme::Instance->Condition()) exactly as before this existed.
struct PlainComponent
{
  void LogSomething(const Logme::ChannelPtr& ch)
  {
    LogmeI(ch, "from PlainComponent");
  }
};

// A free function in the SAME translation unit as FlaggedComponent, and
// even textually after it. Proves the override is scoped to
// FlaggedComponent's own methods (found via ordinary member lookup, which
// only applies inside a member function of a class that has the member)
// and never leaks into unrelated code in the same file.
static void LogFromFreeFunction(const Logme::ChannelPtr& ch)
{
  LogmeI(ch, "from free function");
}

TEST(LoggerConditionOverride, MemberOverrideSuppressesLoggingWhenFalse)
{
  auto ch = MakeChannel("logger_condition_override_suppress", true);
  Be->Clear();

  FlaggedComponent c;
  c.Enabled = false;
  c.LogSomething(ch);

  EXPECT_TRUE(Be->History.empty());
}

TEST(LoggerConditionOverride, MemberOverrideAllowsLoggingWhenTrue)
{
  auto ch = MakeChannel("logger_condition_override_allow", true);
  Be->Clear();

  FlaggedComponent c;
  c.Enabled = true;
  c.LogSomething(ch);

  ASSERT_EQ(Be->History.size(), 1u);
  EXPECT_NE(Be->History[0].find("from FlaggedComponent"), std::string::npos);
}

TEST(LoggerConditionOverride, NonOverridingClassStillUsesGlobalDefault)
{
  auto ch = MakeChannel("logger_condition_override_plain", true);
  Be->Clear();

  PlainComponent c;
  c.LogSomething(ch);

  ASSERT_EQ(Be->History.size(), 1u);
  EXPECT_NE(Be->History[0].find("from PlainComponent"), std::string::npos);
}

TEST(LoggerConditionOverride, FreeFunctionInSameTranslationUnitUnaffectedByOverride)
{
  auto ch = MakeChannel("logger_condition_override_free_fn", true);
  Be->Clear();

  LogFromFreeFunction(ch);

  ASSERT_EQ(Be->History.size(), 1u);
  EXPECT_NE(Be->History[0].find("from free function"), std::string::npos);
}

TEST(LoggerConditionOverride, OverrideIsIndependentOfProcessWideCondition)
{
  auto ch = MakeChannel("logger_condition_override_independent", true);

  // Force the process-wide gate off. Every macro that does NOT go through
  // an override still funnels into this same callback via the default
  // LoggerCondition() body.
  Logme::Instance->SetCondition([]() -> bool { return false; });

  Be->Clear();
  PlainComponent plain;
  plain.LogSomething(ch);
  EXPECT_TRUE(Be->History.empty());

  Be->Clear();
  FlaggedComponent flagged;
  flagged.Enabled = true;
  flagged.LogSomething(ch);
  ASSERT_EQ(Be->History.size(), 1u);
  EXPECT_NE(Be->History[0].find("from FlaggedComponent"), std::string::npos);

  Logme::Instance->SetCondition(nullptr);
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
