#include <gtest/gtest.h>

#include <Logme/Logger.h>

namespace
{
  Logme::SID DslSid()
  {
    return Logme::SID::Build("DSL");
  }

  Logme::SID CloudSid()
  {
    return Logme::SID::Build("CLOUD");
  }
}

TEST(SubsystemLevelConfig, LoadsConfiguredLevels)
{
  Logme::Logger logger;
  std::string error;

  ASSERT_TRUE(logger.LoadConfiguration(
    R"({"subsystems":{"levels":{"DSL":"debug","CLOUD":"WARN"}}})"
    , std::string()
    , &error
  )) << error;

  Logme::Level level = Logme::LEVEL_CRITICAL;
  ASSERT_TRUE(logger.GetSubsystemLevel(DslSid(), level));
  EXPECT_EQ(level, Logme::LEVEL_DEBUG);

  ASSERT_TRUE(logger.GetSubsystemLevel(CloudSid(), level));
  EXPECT_EQ(level, Logme::LEVEL_WARN);
  EXPECT_TRUE(logger.HasSubsystemLevelOverrides());
}

TEST(SubsystemLevelConfig, ReloadReplacesConfiguredLevels)
{
  Logme::Logger logger;
  std::string error;

  ASSERT_TRUE(logger.LoadConfiguration(
    R"({"subsystems":{"levels":{"DSL":"DEBUG","CLOUD":"WARN"}}})"
    , std::string()
    , &error
  )) << error;

  ASSERT_TRUE(logger.LoadConfiguration(
    R"({"subsystems":{"levels":{"DSL":"ERROR"}}})"
    , std::string()
    , &error
  )) << error;

  Logme::Level level = Logme::LEVEL_DEBUG;
  ASSERT_TRUE(logger.GetSubsystemLevel(DslSid(), level));
  EXPECT_EQ(level, Logme::LEVEL_ERROR);
  EXPECT_FALSE(logger.GetSubsystemLevel(CloudSid(), level));
}

TEST(SubsystemLevelConfig, MissingLevelsClearsConfiguredLevels)
{
  Logme::Logger logger;
  std::string error;

  ASSERT_TRUE(logger.LoadConfiguration(
    R"({"subsystems":{"levels":{"DSL":"DEBUG"}}})"
    , std::string()
    , &error
  )) << error;

  ASSERT_TRUE(logger.LoadConfiguration(
    R"({"subsystems":{"blocked":["CLOUD"]}})"
    , std::string()
    , &error
  )) << error;

  Logme::Level level = Logme::LEVEL_DEBUG;
  EXPECT_FALSE(logger.GetSubsystemLevel(DslSid(), level));
  EXPECT_FALSE(logger.HasSubsystemLevelOverrides());
}

TEST(SubsystemLevelConfig, RejectsNonObjectLevels)
{
  Logme::Logger logger;
  std::string error;

  EXPECT_FALSE(logger.LoadConfiguration(
    R"({"subsystems":{"levels":[]}})"
    , std::string()
    , &error
  ));
  EXPECT_EQ(error, "subsystem configuration parsing failed");
}

TEST(SubsystemLevelConfig, RejectsNonStringLevel)
{
  Logme::Logger logger;
  std::string error;

  EXPECT_FALSE(logger.LoadConfiguration(
    R"({"subsystems":{"levels":{"DSL":2}}})"
    , std::string()
    , &error
  ));
  EXPECT_EQ(error, "subsystem configuration parsing failed");
}

TEST(SubsystemLevelConfig, RejectsUnknownLevel)
{
  Logme::Logger logger;
  std::string error;

  EXPECT_FALSE(logger.LoadConfiguration(
    R"({"subsystems":{"levels":{"DSL":"VERBOSE"}}})"
    , std::string()
    , &error
  ));
  EXPECT_EQ(error, "subsystem configuration parsing failed");
}

TEST(SubsystemLevelConfig, RejectsEmptySubsystemName)
{
  Logme::Logger logger;
  std::string error;

  EXPECT_FALSE(logger.LoadConfiguration(
    R"({"subsystems":{"levels":{"":"DEBUG"}}})"
    , std::string()
    , &error
  ));
  EXPECT_EQ(error, "subsystem configuration parsing failed");
}
