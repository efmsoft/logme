#include <Common/TestBackend.h>

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
