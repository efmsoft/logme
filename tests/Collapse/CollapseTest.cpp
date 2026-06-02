#include <Common/TestBackend.h>

#include <gtest/gtest.h>

#include <Logme/Logme.h>

std::shared_ptr<TestBackend> Be;

static bool Contains(const std::string& str, const char* text)
{
  return str.find(text) != std::string::npos;
}

static void LogSameMessage()
{
  LogmeE_Collapse(3, "collapse same message");
}

static void LogIgnoreDigits(int value)
{
  LogmeE_CollapseIgnore("\\d+", 2, "collapse ignore value=%d", value);
}

static void LogAllLevels(int value)
{
  LogmeD_Collapse(1, "debug collapse");
  LogmeI_Collapse(1, "info collapse");
  LogmeW_Collapse(1, "warn collapse");
  LogmeE_Collapse(1, "error collapse");
  LogmeC_Collapse(1, "critical collapse");

  LogmeD_CollapseIgnore("\\d+", 1, "debug collapse ignore %d", value);
  LogmeI_CollapseIgnore("\\d+", 1, "info collapse ignore %d", value);
  LogmeW_CollapseIgnore("\\d+", 1, "warn collapse ignore %d", value);
  LogmeE_CollapseIgnore("\\d+", 1, "error collapse ignore %d", value);
  LogmeC_CollapseIgnore("\\d+", 1, "critical collapse ignore %d", value);
}

TEST(Collapse, RepeatedMessageIsCollapsed)
{
  Be->Clear();

  for (int i = 0; i < 4; i++)
    LogSameMessage();

  ASSERT_EQ(Be->History.size(), 2u);
  EXPECT_TRUE(Contains(Be->History[0], "collapse same message"));
  EXPECT_TRUE(Contains(Be->History[1], "repeated 3 times: collapse same message"));
}

TEST(Collapse, IgnoreRegexCollapsesNormalizedMessages)
{
  Be->Clear();

  LogIgnoreDigits(1);
  LogIgnoreDigits(2);
  LogIgnoreDigits(3);

  ASSERT_EQ(Be->History.size(), 2u);
  EXPECT_TRUE(Contains(Be->History[0], "collapse ignore value=1"));
  EXPECT_TRUE(Contains(Be->History[1], "repeated 2 times: collapse ignore value=3"));
}

TEST(Collapse, AllLevelsAreAvailable)
{
  Be->Clear();

  LogAllLevels(1);
  LogAllLevels(2);

  ASSERT_EQ(Be->History.size(), 20u);
  EXPECT_TRUE(Contains(Be->History[10], "repeated 1 times: debug collapse"));
  EXPECT_TRUE(Contains(Be->History[11], "repeated 1 times: info collapse"));
  EXPECT_TRUE(Contains(Be->History[12], "repeated 1 times: warn collapse"));
  EXPECT_TRUE(Contains(Be->History[13], "repeated 1 times: error collapse"));
  EXPECT_TRUE(Contains(Be->History[14], "repeated 1 times: critical collapse"));
  EXPECT_TRUE(Contains(Be->History[15], "repeated 1 times: debug collapse ignore 2"));
  EXPECT_TRUE(Contains(Be->History[16], "repeated 1 times: info collapse ignore 2"));
  EXPECT_TRUE(Contains(Be->History[17], "repeated 1 times: warn collapse ignore 2"));
  EXPECT_TRUE(Contains(Be->History[18], "repeated 1 times: error collapse ignore 2"));
  EXPECT_TRUE(Contains(Be->History[19], "repeated 1 times: critical collapse ignore 2"));
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  auto ch = Logme::Instance->GetExistingChannel(CH);
  ch->SetFilterLevel(Logme::LEVEL_DEBUG);
  ch->RemoveBackends();
  Be = std::make_shared<TestBackend>(ch);
  ch->AddBackend(Be);

  Logme::OutputFlags flags;
  flags.Value = 0;
  ch->SetFlags(flags);

  return RUN_ALL_TESTS();
}
