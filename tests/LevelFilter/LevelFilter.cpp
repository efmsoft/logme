#include <Common/TestBackend.h>
#include <Common/TestData.h>

#include <gtest/gtest.h>

#include <Logme/Logme.h>

Logme::ID CHT{ "level_filter" };
std::shared_ptr<TestBackend> Be;

TEST(OutputFlags, LEVEL_DEBUG)
{
  Be->Owner->SetFilterLevel(Logme::LEVEL_DEBUG);

  Be->Clear();
  LogmeD(CHT, Lorem);
  EXPECT_EQ(Be->Line, Lorem);

  Be->Clear();
  LogmeI(CHT, Lorem);
  EXPECT_EQ(Be->Line, Lorem);

  Be->Clear();
  LogmeW(CHT, Lorem);
  EXPECT_EQ(Be->Line, Lorem);

  Be->Clear();
  LogmeE(CHT, Lorem);
  EXPECT_EQ(Be->Line, Lorem);

  Be->Clear();
  LogmeC(CHT, Lorem);
  EXPECT_EQ(Be->Line, Lorem);
}

TEST(OutputFlags, LEVEL_INFO)
{
  Be->Owner->SetFilterLevel(Logme::LEVEL_INFO);

  Be->Clear();
  LogmeD(CHT, Lorem);
  EXPECT_EQ(Be->Line, "");

  Be->Clear();
  LogmeI(CHT, Lorem);
  EXPECT_EQ(Be->Line, Lorem);

  Be->Clear();
  LogmeW(CHT, Lorem);
  EXPECT_EQ(Be->Line, Lorem);

  Be->Clear();
  LogmeE(CHT, Lorem);
  EXPECT_EQ(Be->Line, Lorem);

  Be->Clear();
  LogmeC(CHT, Lorem);
  EXPECT_EQ(Be->Line, Lorem);
}

TEST(OutputFlags, LEVEL_WARN)
{
  Be->Owner->SetFilterLevel(Logme::LEVEL_WARN);

  Be->Clear();
  LogmeD(CHT, Lorem);
  EXPECT_EQ(Be->Line, "");

  Be->Clear();
  LogmeI(CHT, Lorem);
  EXPECT_EQ(Be->Line, "");

  Be->Clear();
  LogmeW(CHT, Lorem);
  EXPECT_EQ(Be->Line, Lorem);

  Be->Clear();
  LogmeE(CHT, Lorem);
  EXPECT_EQ(Be->Line, Lorem);

  Be->Clear();
  LogmeC(CHT, Lorem);
  EXPECT_EQ(Be->Line, Lorem);
}

TEST(OutputFlags, LEVEL_ERROR)
{
  Be->Owner->SetFilterLevel(Logme::LEVEL_ERROR);

  Be->Clear();
  LogmeD(CHT, Lorem);
  EXPECT_EQ(Be->Line, "");

  Be->Clear();
  LogmeI(CHT, Lorem);
  EXPECT_EQ(Be->Line, "");

  Be->Clear();
  LogmeW(CHT, Lorem);
  EXPECT_EQ(Be->Line, "");

  Be->Clear();
  LogmeE(CHT, Lorem);
  EXPECT_EQ(Be->Line, Lorem);

  Be->Clear();
  LogmeC(CHT, Lorem);
  EXPECT_EQ(Be->Line, Lorem);
}

TEST(OutputFlags, LEVEL_CRITICAL)
{
  Be->Owner->SetFilterLevel(Logme::LEVEL_CRITICAL);

  Be->Clear();
  LogmeD(CHT, Lorem);
  EXPECT_EQ(Be->Line, "");

  Be->Clear();
  LogmeI(CHT, Lorem);
  EXPECT_EQ(Be->Line, "");

  Be->Clear();
  LogmeW(CHT, Lorem);
  EXPECT_EQ(Be->Line, "");

  Be->Clear();
  LogmeE(CHT, Lorem);
  EXPECT_EQ(Be->Line, "");

  Be->Clear();
  LogmeC(CHT, Lorem);
  EXPECT_EQ(Be->Line, Lorem);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  auto ch = Logme::Instance->CreateChannel(CHT);
  Be = std::make_shared<TestBackend>(ch);
  ch->AddBackend(Be);

  Logme::OutputFlags flags;
  flags.Value = 0;
  ch->SetFlags(flags);

  return RUN_ALL_TESTS();
}

