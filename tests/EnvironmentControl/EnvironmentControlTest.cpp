#include <Common/TestBackend.h>

#include <gtest/gtest.h>

#include <Logme/Logme.h>

#include <cstdlib>
#include <memory>
#include <string>

using namespace Logme;

ID CHT{ "environment_control" };
std::shared_ptr<TestBackend> Be;
ChannelPtr Ch;

static void SetEnvironmentVariableForTest(
  const std::string& name
  , const std::string& value
)
{
#ifdef _WIN32
  _putenv_s(name.c_str(), value.c_str());
#else
  setenv(name.c_str(), value.c_str(), 1);
#endif
}

static void RemoveEnvironmentVariableForTest(const std::string& name)
{
#ifdef _WIN32
  _putenv_s(name.c_str(), "");
#else
  unsetenv(name.c_str());
#endif
}

static void ClearEnvironmentVariables()
{
  RemoveEnvironmentVariableForTest("LOGME_TEST_CONTROL");

  for (int i = 1; i <= 32; ++i)
  {
    RemoveEnvironmentVariableForTest(
      "LOGME_TEST_CONTROL_" + std::to_string(i)
    );
  }
}

static void ResetLoggerState()
{
  ClearEnvironmentVariables();

  Ch->SetFilterLevel(Logme::LEVEL_INFO);
  Ch->SetEnabled(true);

  (void)Logme::Instance->Control(
    "backend --channel environment_control --delete ring"
  );

  Be->Clear();
}

static EnvironmentControlOptions MakeOptions()
{
  EnvironmentControlOptions options;

  options.Prefix = "LOGME_TEST";
  options.Policy = ControlPolicy::Safe();
  options.ErrorMode = EnvironmentControlErrorMode::CONTINUE_ON_ERROR;
  options.LogAppliedCommands = false;

  return options;
}

static bool Contains(
  const std::string& text
  , const std::string& value
)
{
  return text.find(value) != std::string::npos;
}

TEST(EnvironmentControl, EnvironmentIsIgnoredWithoutExplicitCall)
{
  ResetLoggerState();

  SetEnvironmentVariableForTest(
    "LOGME_TEST_CONTROL"
    , "level --channel environment_control debug"
  );

  LogmeD(CHT, "debug before explicit environment control call");

  EXPECT_TRUE(Be->History.empty());
  EXPECT_EQ(Ch->GetFilterLevel(), Logme::LEVEL_INFO);
}

TEST(EnvironmentControl, AppliesMultipleCommandsSeparatedBySemicolon)
{
  ResetLoggerState();

  SetEnvironmentVariableForTest(
    "LOGME_TEST_CONTROL"
    , "level --channel environment_control debug; channel --disable environment_control"
  );

  EnvironmentControlOptions options = MakeOptions();
  EXPECT_TRUE(Logme::Instance->ApplyEnvironmentControl(options));

  EXPECT_EQ(Ch->GetFilterLevel(), Logme::LEVEL_DEBUG);
  EXPECT_FALSE(Ch->GetEnabled());
}

TEST(EnvironmentControl, ContinueOnErrorKeepsProcessingAfterRejectedCommand)
{
  ResetLoggerState();

  SetEnvironmentVariableForTest(
    "LOGME_TEST_CONTROL"
    , "level --channel environment_control debug; backend --channel environment_control --add ring --max-items 3; channel --disable environment_control"
  );

  EnvironmentControlOptions options = MakeOptions();
  EXPECT_FALSE(Logme::Instance->ApplyEnvironmentControl(options));

  EXPECT_EQ(Ch->GetFilterLevel(), Logme::LEVEL_DEBUG);
  EXPECT_FALSE(Ch->GetEnabled());

  std::string response = Logme::Instance->Control(
    "backend --channel environment_control --delete ring"
  );

  EXPECT_TRUE(Contains(response, "error:"));
}

TEST(EnvironmentControl, StopOnErrorStopsProcessingAfterRejectedCommand)
{
  ResetLoggerState();

  SetEnvironmentVariableForTest(
    "LOGME_TEST_CONTROL"
    , "backend --channel environment_control --add ring --max-items 3; level --channel environment_control debug"
  );

  EnvironmentControlOptions options = MakeOptions();
  options.ErrorMode = EnvironmentControlErrorMode::STOP_ON_ERROR;

  EXPECT_FALSE(Logme::Instance->ApplyEnvironmentControl(options));
  EXPECT_EQ(Ch->GetFilterLevel(), Logme::LEVEL_INFO);

  std::string response = Logme::Instance->Control(
    "backend --channel environment_control --delete ring"
  );

  EXPECT_TRUE(Contains(response, "error:"));
}

TEST(EnvironmentControl, DiagnosticPolicyAllowsRingBufferBackend)
{
  ResetLoggerState();

  SetEnvironmentVariableForTest(
    "LOGME_TEST_CONTROL"
    , "backend --channel environment_control --add ring --max-items 3; level --channel environment_control debug"
  );

  EnvironmentControlOptions options = MakeOptions();
  options.Policy = ControlPolicy::Diagnostic();

  EXPECT_TRUE(Logme::Instance->ApplyEnvironmentControl(options));
  EXPECT_EQ(Ch->GetFilterLevel(), Logme::LEVEL_DEBUG);

  std::string response = Logme::Instance->Control(
    "backend --channel environment_control --delete ring"
  );

  EXPECT_EQ(response, "ok");
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  Ch = Logme::Instance->CreateChannel(CHT);
  Be = std::make_shared<TestBackend>(Ch);
  Ch->AddBackend(Be);
  Ch->SetFilterLevel(Logme::LEVEL_INFO);

  Logme::OutputFlags flags;
  flags.Value = 0;
  Ch->SetFlags(flags);

  int result = RUN_ALL_TESTS();

  ClearEnvironmentVariables();
  return result;
}
