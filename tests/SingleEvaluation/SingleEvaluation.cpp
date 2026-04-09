#include <Common/TestBackend.h>

#include <gtest/gtest.h>

#include <Logme/Logme.h>

std::shared_ptr<TestBackend> Be;

TEST(SingleEvaluation, Logme)
{
  Be->Owner->SetFilterLevel(Logme::LEVEL_INFO);
  Be->Clear();

  int i = 0;
  for (; i < 10;)
    LogmeI("i=%i", i++);

  EXPECT_EQ(i, 10);
  EXPECT_EQ(Be->History.size(), 10u);
}

#ifndef LOGME_DISABLE_STD_FORMAT
TEST(SingleEvaluation, StdFormat)
{
  Be->Owner->SetFilterLevel(Logme::LEVEL_INFO);
  Be->Clear();

  int i = 0;
  for (; i < 10;)
    fLogmeI("i={}", i++);

  EXPECT_EQ(i, 10);
  EXPECT_EQ(Be->History.size(), 10u);
}
#endif

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  auto ch = Logme::Instance->GetExistingChannel(CH);
  ch->RemoveBackends();
  Be = std::make_shared<TestBackend>(ch);
  ch->AddBackend(Be);

  Logme::OutputFlags flags;
  flags.Value = 0;
  ch->SetFlags(flags);

  return RUN_ALL_TESTS();
}
