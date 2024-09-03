#include <Common/TestBackend.h>
#include <Common/TestData.h>

#include <gtest/gtest.h>

#include <Logme/Logme.h>

Logme::ID CHT{ "ch-redirect" };
std::shared_ptr<TestBackend> Be;
std::shared_ptr<TestBackend> Ge;

TEST(ChannelRedirect, Global)
{
  Be->Owner->SetFilterLevel(Logme::LEVEL_DEBUG);
  Ge->Owner->SetFilterLevel(Logme::LEVEL_DEBUG);

  // Must be written to ::CH
  Be->Clear();
  Ge->Clear();
  LogmeD(Lorem);
  EXPECT_EQ(Be->Line, "");
  EXPECT_EQ(Ge->Line, Lorem);

  Be->Clear();
  Ge->Clear();
  LogmeI() << Lorem;
  EXPECT_EQ(Be->Line, "");
  EXPECT_EQ(Ge->Line, Lorem);

  Be->Clear();
  Ge->Clear();
  fLogmeW("{}", Lorem);
  EXPECT_EQ(Be->Line, "");
  EXPECT_EQ(Ge->Line, Lorem);
}

TEST(ChannelRedirect, Local)
{
  Be->Owner->SetFilterLevel(Logme::LEVEL_INFO);
  Ge->Owner->SetFilterLevel(Logme::LEVEL_INFO);

  Logme::ID CH = CHT;

  // Must be written to local CH
  Be->Clear();
  Ge->Clear();
  LogmeI(Lorem);
  EXPECT_EQ(Be->Line, Lorem);
  EXPECT_EQ(Ge->Line, "");

  Be->Clear();
  Ge->Clear();
  LogmeW() << Lorem;
  EXPECT_EQ(Be->Line, Lorem);
  EXPECT_EQ(Ge->Line, "");

  CH = ::CH;
  Be->Clear();
  Ge->Clear();
  fLogmeE("{}", Lorem);
  EXPECT_EQ(Be->Line, "");
  EXPECT_EQ(Ge->Line, Lorem);
}

TEST(ChannelRedirect, Custom)
{
  Be->Owner->SetFilterLevel(Logme::LEVEL_INFO);
  Ge->Owner->SetFilterLevel(Logme::LEVEL_INFO);

  // Must be written to local CH
  Be->Clear();
  Ge->Clear();
  LogmeI(CHT, Lorem);
  EXPECT_EQ(Be->Line, Lorem);
  EXPECT_EQ(Ge->Line, "");

  Be->Clear();
  Ge->Clear();
  LogmeW(CHT) << Lorem;
  EXPECT_EQ(Be->Line, Lorem);
  EXPECT_EQ(Ge->Line, "");

  Be->Clear();
  Ge->Clear();
  fLogmeE(CHT, "{}", Lorem);
  EXPECT_EQ(Be->Line, Lorem);
  EXPECT_EQ(Ge->Line, "");
}

TEST(ChannelRedirect, Override)
{
  Logme::Override ovr;
  Be->Owner->SetFilterLevel(Logme::LEVEL_INFO);
  Ge->Owner->SetFilterLevel(Logme::LEVEL_INFO);

  // Must be written to local CH
  Be->Clear();
  Ge->Clear();
  LogmeI(CHT, ovr, Lorem);
  EXPECT_EQ(Be->Line, Lorem);
  EXPECT_EQ(Ge->Line, "");

  Be->Clear();
  Ge->Clear();
  LogmeW(CHT, ovr) << Lorem;
  EXPECT_EQ(Be->Line, Lorem);
  EXPECT_EQ(Ge->Line, "");

  Be->Clear();
  Ge->Clear();
  fLogmeE(CHT, ovr, "{}", Lorem);
  EXPECT_EQ(Be->Line, Lorem);
  EXPECT_EQ(Ge->Line, "");
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  auto ge = Logme::Instance->GetExistingChannel(CH);
  ge->RemoveBackends();
  Ge = std::make_shared<TestBackend>(ge);
  ge->AddBackend(Ge);

  auto ch = Logme::Instance->CreateChannel(CHT);
  Be = std::make_shared<TestBackend>(ch);
  ch->AddBackend(Be);

  Logme::OutputFlags flags;
  flags.Value = 0;
  ch->SetFlags(flags);
  ge->SetFlags(flags);

  return RUN_ALL_TESTS();
}

