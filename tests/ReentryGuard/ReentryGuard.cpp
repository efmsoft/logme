#include <Common/TestBackend.h>
#include <Common/TestData.h>

#include <gtest/gtest.h>

#include <Logme/Logme.h>

using namespace Logme;

namespace
{
  ID CHA{ "reentry_a" };
  ID CHB{ "reentry_b" };
  ID CHC{ "reentry_c" };
  ID CHD{ "reentry_d" };
  ID CHE{ "reentry_e" };

  std::shared_ptr<TestBackend> BackendA;
  std::shared_ptr<TestBackend> BackendB;
  std::shared_ptr<TestBackend> BackendC;
  std::shared_ptr<TestBackend> BackendD;
  std::shared_ptr<TestBackend> BackendE;

  class ForwardToChannelBackend : public Backend
  {
  public:
    ForwardToChannelBackend(
      ChannelPtr owner
      , const ID &target
      , const char *text
    )
      : Backend(owner, "ForwardToChannelBackend")
      , Target(target)
      , Text(text ? text : "")
      , CallCount(0)
    {
    }

    void Display(Context &context, const char *line) override
    {
      (void)context;
      (void)line;

      ++CallCount;
      LogmeI(Target, "%s", Text.c_str());
    }

    ID Target;
    std::string Text;
    int CallCount;
  };

  class ReenterSameChannelBackend : public Backend
  {
  public:
    ReenterSameChannelBackend(
      ChannelPtr owner
      , const ID &target
      , const char *text
    )
      : Backend(owner, "ReenterSameChannelBackend")
      , Target(target)
      , Text(text ? text : "")
      , ReentryAttemptCount(0)
      , Triggered(false)
    {
    }

    void Display(Context &context, const char *line) override
    {
      (void)context;
      (void)line;

      ++ReentryAttemptCount;

      if (Triggered)
      {
        return;
      }

      Triggered = true;
      LogmeE(Target, "%s", Text.c_str());
    }

    void Reset()
    {
      ReentryAttemptCount = 0;
      Triggered = false;
    }

    ID Target;
    std::string Text;
    int ReentryAttemptCount;
    bool Triggered;
  };

  std::shared_ptr<ForwardToChannelBackend> ForwardBackend;
  std::shared_ptr<ReenterSameChannelBackend> ReenterBackend;
}

TEST(ReentryGuard, SameChannelIsBlockedButLinkWorks)
{
  BackendA->Clear();
  BackendB->Clear();
  BackendC->Clear();
  ReenterBackend->Reset();
  ForwardBackend->CallCount = 0;

  LogmeI(CHA, Lorem);

  ASSERT_EQ(BackendA->History.size(), 1u);
  EXPECT_EQ(BackendA->History[0], Lorem);

  ASSERT_EQ(BackendB->History.size(), 1u);
  EXPECT_EQ(BackendB->History[0], Lorem);

  ASSERT_EQ(BackendC->History.size(), 1u);
  EXPECT_EQ(BackendC->History[0], "forwarded-to-c");

  EXPECT_EQ(ForwardBackend->CallCount, 1);
  EXPECT_EQ(ReenterBackend->ReentryAttemptCount, 1);
}

TEST(ReentryGuard, LinkCycleIsBlocked)
{
  BackendD->Clear();
  BackendE->Clear();

  LogmeI(CHD, Lorem);

  ASSERT_EQ(BackendD->History.size(), 1u);
  EXPECT_EQ(BackendD->History[0], Lorem);

  ASSERT_EQ(BackendE->History.size(), 1u);
  EXPECT_EQ(BackendE->History[0], Lorem);
}

int main(int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  auto chA = Instance->CreateChannel(CHA);
  auto chB = Instance->CreateChannel(CHB);
  auto chC = Instance->CreateChannel(CHC);
  auto chD = Instance->CreateChannel(CHD);
  auto chE = Instance->CreateChannel(CHE);

  BackendA = std::make_shared<TestBackend>(chA);
  BackendB = std::make_shared<TestBackend>(chB);
  BackendC = std::make_shared<TestBackend>(chC);
  BackendD = std::make_shared<TestBackend>(chD);
  BackendE = std::make_shared<TestBackend>(chE);

  ForwardBackend = std::make_shared<ForwardToChannelBackend>(
    chA
    , CHC
    , "forwarded-to-c"
  );

  ReenterBackend = std::make_shared<ReenterSameChannelBackend>(
    chC
    , CHC
    , "reentered-c"
  );

  chA->AddBackend(BackendA);
  chA->AddBackend(ForwardBackend);

  chB->AddBackend(BackendB);

  chC->AddBackend(BackendC);
  chC->AddBackend(ReenterBackend);

  chD->AddBackend(BackendD);
  chE->AddBackend(BackendE);

  chA->AddLink(chB);

  chD->AddLink(chE);
  chE->AddLink(chD);

  OutputFlags flags;
  flags.Value = 0;

  chA->SetFlags(flags);
  chB->SetFlags(flags);
  chC->SetFlags(flags);
  chD->SetFlags(flags);
  chE->SetFlags(flags);

  chA->SetFilterLevel(LEVEL_DEBUG);
  chB->SetFilterLevel(LEVEL_DEBUG);
  chC->SetFilterLevel(LEVEL_DEBUG);
  chD->SetFilterLevel(LEVEL_DEBUG);
  chE->SetFilterLevel(LEVEL_DEBUG);

  return RUN_ALL_TESTS();
}