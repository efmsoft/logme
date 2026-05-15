#include <Common/TestBackend.h>

#include <gtest/gtest.h>

#include <Logme/ArgumentList.h>
#include <Logme/Logme.h>

#include <string>

using namespace Logme;

ID CHT{ "trace_points" };
std::shared_ptr<TestBackend> Be;

static bool Contains(const std::string& text, const std::string& value)
{
  return text.find(value) != std::string::npos;
}

static void TracePointWithMessage(int value)
{
  LogmeTPt(CHT, "trace point value=%d", value);
}

static void TracePointWithoutMessage()
{
  LogmeTPt(CHT);
}

static void TracePointWithArgs(int first, int second)
{
  LogmeTPt(CHT, "trace point args: %s", ARGS2(first, second));
}

static void RegisterTracePoints()
{
  Logme::Instance->SetTracePointsEnabled("*", false);

  TracePointWithMessage(0);
  TracePointWithoutMessage();
  TracePointWithArgs(0, 0);

  Logme::Instance->ResetTracePointCounters("");
  Be->Clear();
}

TEST(TracePoints, DisabledPointCountsHitsButDoesNotWriteLog)
{
  RegisterTracePoints();
  Logme::Instance->SetTracePointsEnabled("*", false);

  TracePointWithMessage(10);

  EXPECT_TRUE(Be->History.empty());

  std::string dump = Logme::Instance->DumpTracePoints("*:TracePointWithMessage:*");
  EXPECT_TRUE(Contains(dump, "off"));
  EXPECT_TRUE(Contains(dump, "hits=1"));
}

TEST(TracePoints, EnabledPointWritesThroughLogPipeline)
{
  RegisterTracePoints();
  Logme::Instance->SetTracePointsEnabled("*:TracePointWithMessage:*", true);

  TracePointWithMessage(20);

  ASSERT_EQ(Be->History.size(), 1u);
  EXPECT_TRUE(Contains(Be->Line, "trace point value=20"));

  std::string dump = Logme::Instance->DumpTracePoints("*:TracePointWithMessage:*");
  EXPECT_TRUE(Contains(dump, "on"));
  EXPECT_TRUE(Contains(dump, "hits=1"));
}

TEST(TracePoints, WildcardControlsOnlyMatchedPoints)
{
  RegisterTracePoints();
  Logme::Instance->SetTracePointsEnabled("*", false);
  Logme::Instance->SetTracePointsEnabled("*:TracePointWithoutMessage:*", true);

  TracePointWithMessage(30);
  TracePointWithoutMessage();

  ASSERT_EQ(Be->History.size(), 1u);
  EXPECT_TRUE(Contains(Be->Line, "TracePointWithoutMessage"));

  std::string messageDump = Logme::Instance->DumpTracePoints("*:TracePointWithMessage:*");
  std::string emptyDump = Logme::Instance->DumpTracePoints("*:TracePointWithoutMessage:*");

  EXPECT_TRUE(Contains(messageDump, "off"));
  EXPECT_TRUE(Contains(messageDump, "hits=1"));
  EXPECT_TRUE(Contains(emptyDump, "on"));
  EXPECT_TRUE(Contains(emptyDump, "hits=1"));
}

TEST(TracePoints, ArgumentListCanBeUsedInTracePointMessage)
{
  RegisterTracePoints();
  Logme::Instance->SetTracePointsEnabled("*:TracePointWithArgs:*", true);

  TracePointWithArgs(7, 9);

  ASSERT_EQ(Be->History.size(), 1u);
  EXPECT_TRUE(Contains(Be->Line, "first=7"));
  EXPECT_TRUE(Contains(Be->Line, "second=9"));
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  auto ch = Logme::Instance->CreateChannel(CHT);
  Be = std::make_shared<TestBackend>(ch);
  ch->AddBackend(Be);
  ch->SetFilterLevel(Logme::LEVEL_INFO);

  Logme::OutputFlags flags;
  flags.Value = 0;
  flags.Location = Logme::DETALITY_SHORT;
  flags.Method = true;
  ch->SetFlags(flags);

  return RUN_ALL_TESTS();
}
