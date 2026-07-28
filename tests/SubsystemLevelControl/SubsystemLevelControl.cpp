#include <gtest/gtest.h>

#include <Logme/Logme.h>

#include <string>

using namespace Logme;

namespace
{
  bool Contains(const std::string& text, const std::string& value)
  {
    return text.find(value) != std::string::npos;
  }

  void ResetSubsystems()
  {
    EXPECT_EQ(Instance->Control("subsystem --clear"), "ok");
  }
}

TEST(SubsystemLevelControl, SetsListsChecksAndRemovesLevels)
{
  ResetSubsystems();

  EXPECT_EQ(Instance->Control("subsystem --set-level DSL debug"), "ok");
  EXPECT_EQ(Instance->Control("subsystem --set-level CLOUD WARN"), "ok");

  Level level = LEVEL_INFO;
  ASSERT_TRUE(Instance->GetSubsystemLevel(SID::Build("DSL"), level));
  EXPECT_EQ(level, LEVEL_DEBUG);
  ASSERT_TRUE(Instance->GetSubsystemLevel(SID::Build("CLOUD"), level));
  EXPECT_EQ(level, LEVEL_WARN);

  std::string list = Instance->Control("subsystem");
  EXPECT_TRUE(Contains(list, "Subsystem levels:"));
  EXPECT_TRUE(Contains(list, "DSL: DEBUG"));
  EXPECT_TRUE(Contains(list, "CLOUD: WARN"));

  std::string check = Instance->Control("subsystem --check DSL");
  EXPECT_TRUE(Contains(check, "Blocked: false"));
  EXPECT_TRUE(Contains(check, "Allowed: false"));
  EXPECT_TRUE(Contains(check, "Level: DEBUG"));

  EXPECT_EQ(Instance->Control("subsystem --remove-level DSL"), "ok");
  EXPECT_FALSE(Instance->GetSubsystemLevel(SID::Build("DSL"), level));
  EXPECT_TRUE(Instance->HasSubsystemLevelOverrides());

  EXPECT_EQ(Instance->Control("subsystem --clear-levels"), "ok");
  EXPECT_FALSE(Instance->HasSubsystemLevelOverrides());
}

TEST(SubsystemLevelControl, ReplacesLevelAndReportsChannelDefault)
{
  ResetSubsystems();

  EXPECT_EQ(Instance->Control("subsystem --set-level DSL DEBUG"), "ok");
  EXPECT_EQ(Instance->Control("subsystem --set-level DSL ERROR"), "ok");

  Level level = LEVEL_INFO;
  ASSERT_TRUE(Instance->GetSubsystemLevel(SID::Build("DSL"), level));
  EXPECT_EQ(level, LEVEL_ERROR);

  EXPECT_EQ(Instance->Control("subsystem --remove-level DSL"), "ok");
  EXPECT_TRUE(Contains(
    Instance->Control("subsystem --check DSL")
    , "Level: channel default"
  ));
}

TEST(SubsystemLevelControl, RejectsInvalidArguments)
{
  ResetSubsystems();

  EXPECT_TRUE(Contains(
    Instance->Control("subsystem --set-level DSL VERBOSE")
    , "error: invalid level"
  ));
  EXPECT_TRUE(Contains(
    Instance->Control("subsystem --remove-level DSL")
    , "error: no level override"
  ));
}

TEST(SubsystemLevelControl, JsonIncludesLevelsAndCheckFields)
{
  ResetSubsystems();

  EXPECT_EQ(Instance->Control("subsystem --block CLOUD"), "ok");
  EXPECT_EQ(Instance->Control("subsystem --allow DSL"), "ok");
  EXPECT_EQ(Instance->Control("subsystem --set-level DSL DEBUG"), "ok");

  std::string listJson = Instance->Control("format json subsystem");
  EXPECT_TRUE(Contains(listJson, "\"blockedSubsystems\":[\"CLOUD\"]"));
  EXPECT_TRUE(Contains(listJson, "\"allowedSubsystems\":[\"DSL\"]"));
  EXPECT_TRUE(Contains(listJson, "\"subsystemLevels\":{\"DSL\":\"DEBUG\"}"));

  std::string checkJson = Instance->Control("format json subsystem --check DSL");
  EXPECT_TRUE(Contains(checkJson, "\"blocked\":false"));
  EXPECT_TRUE(Contains(checkJson, "\"allowed\":true"));
  EXPECT_TRUE(Contains(checkJson, "\"level\":\"DEBUG\""));
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();
  (void)Instance->Control("subsystem --clear");
  return result;
}
