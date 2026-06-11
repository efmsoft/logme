#include <chrono>
#include <thread>

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

static void LogSameMessageEvery()
{
  LogmeE_CollapseEvery(50, "collapse every same message");
}

static void LogIgnoreDigitsEvery(int value)
{
  LogmeE_CollapseIgnoreEvery("\\d+", 50, "collapse ignore every value=%d", value);
}

static void LogEveryKeyA()
{
  LogmeE_CollapseEvery(5000, "collapse every key A");
}

static void LogEveryKeyB()
{
  LogmeE_CollapseEvery(5000, "collapse every key B");
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

  LogmeD_CollapseEvery(1, "debug collapse every");
  LogmeI_CollapseEvery(1, "info collapse every");
  LogmeW_CollapseEvery(1, "warn collapse every");
  LogmeE_CollapseEvery(1, "error collapse every");
  LogmeC_CollapseEvery(1, "critical collapse every");

  LogmeD_CollapseIgnoreEvery("\\d+", 1, "debug collapse ignore every %d", value);
  LogmeI_CollapseIgnoreEvery("\\d+", 1, "info collapse ignore every %d", value);
  LogmeW_CollapseIgnoreEvery("\\d+", 1, "warn collapse ignore every %d", value);
  LogmeE_CollapseIgnoreEvery("\\d+", 1, "error collapse ignore every %d", value);
  LogmeC_CollapseIgnoreEvery("\\d+", 1, "critical collapse ignore every %d", value);
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

TEST(Collapse, TimeBasedRepeatedMessageIsCollapsed)
{
  Be->Clear();

  LogSameMessageEvery();
  LogSameMessageEvery();
  LogSameMessageEvery();

  ASSERT_EQ(Be->History.size(), 1u);
  EXPECT_TRUE(Contains(Be->History[0], "collapse every same message"));

  std::this_thread::sleep_for(std::chrono::milliseconds(70));

  LogSameMessageEvery();

  ASSERT_EQ(Be->History.size(), 2u);
  EXPECT_TRUE(Contains(Be->History[1], "repeated 2 times: collapse every same message"));
}

TEST(Collapse, TimeBasedIgnoreRegexCollapsesNormalizedMessages)
{
  Be->Clear();

  LogIgnoreDigitsEvery(1);
  LogIgnoreDigitsEvery(2);
  LogIgnoreDigitsEvery(3);

  ASSERT_EQ(Be->History.size(), 1u);
  EXPECT_TRUE(Contains(Be->History[0], "collapse ignore every value=1"));

  std::this_thread::sleep_for(std::chrono::milliseconds(70));

  LogIgnoreDigitsEvery(4);

  ASSERT_EQ(Be->History.size(), 2u);
  EXPECT_TRUE(Contains(Be->History[1], "repeated 2 times: collapse ignore every value=4"));
}

TEST(Collapse, TimeBasedKeyChangeDoesNotFlushPreviousSummary)
{
  Be->Clear();

  LogEveryKeyA();
  LogEveryKeyA();
  LogEveryKeyA();
  LogEveryKeyB();

  ASSERT_EQ(Be->History.size(), 2u);
  EXPECT_TRUE(Contains(Be->History[0], "collapse every key A"));
  EXPECT_TRUE(Contains(Be->History[1], "collapse every key B"));
  EXPECT_FALSE(Contains(Be->History[1], "repeated"));
}

TEST(Collapse, AllLevelsAreAvailable)
{
  Be->Clear();

  LogAllLevels(1);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  LogAllLevels(2);

  ASSERT_EQ(Be->History.size(), 40u);
  EXPECT_TRUE(Contains(Be->History[20], "repeated 1 times: debug collapse"));
  EXPECT_TRUE(Contains(Be->History[21], "repeated 1 times: info collapse"));
  EXPECT_TRUE(Contains(Be->History[22], "repeated 1 times: warn collapse"));
  EXPECT_TRUE(Contains(Be->History[23], "repeated 1 times: error collapse"));
  EXPECT_TRUE(Contains(Be->History[24], "repeated 1 times: critical collapse"));
  EXPECT_TRUE(Contains(Be->History[25], "repeated 1 times: debug collapse ignore 2"));
  EXPECT_TRUE(Contains(Be->History[26], "repeated 1 times: info collapse ignore 2"));
  EXPECT_TRUE(Contains(Be->History[27], "repeated 1 times: warn collapse ignore 2"));
  EXPECT_TRUE(Contains(Be->History[28], "repeated 1 times: error collapse ignore 2"));
  EXPECT_TRUE(Contains(Be->History[29], "repeated 1 times: critical collapse ignore 2"));
  EXPECT_TRUE(Contains(Be->History[30], "debug collapse every"));
  EXPECT_TRUE(Contains(Be->History[31], "info collapse every"));
  EXPECT_TRUE(Contains(Be->History[32], "warn collapse every"));
  EXPECT_TRUE(Contains(Be->History[33], "error collapse every"));
  EXPECT_TRUE(Contains(Be->History[34], "critical collapse every"));
  EXPECT_TRUE(Contains(Be->History[35], "debug collapse ignore every 2"));
  EXPECT_TRUE(Contains(Be->History[36], "info collapse ignore every 2"));
  EXPECT_TRUE(Contains(Be->History[37], "warn collapse ignore every 2"));
  EXPECT_TRUE(Contains(Be->History[38], "error collapse ignore every 2"));
  EXPECT_TRUE(Contains(Be->History[39], "critical collapse ignore every 2"));
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
