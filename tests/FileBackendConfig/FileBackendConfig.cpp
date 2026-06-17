#include <gtest/gtest.h>

#include <Logme/Backend/FileBackend.h>

#include <json/json.h>

namespace
{
  Json::Value MakeFileConfig()
  {
    Json::Value config;
    config["file"] = "file-backend-config-test.log";
    return config;
  }
}

TEST(FileBackendConfigTest, AcceptsAllTimeRotationModes)
{
  struct TestCase
  {
    const char* Value;
    Logme::TimeRotationMode Mode;
    bool DailyCompatibility;
  };

  const TestCase cases[] =
  {
    { "hourly", Logme::TIME_ROTATION_HOURLY, false },
    { "daily", Logme::TIME_ROTATION_DAILY, true },
    { "weekly", Logme::TIME_ROTATION_WEEKLY, false },
    { "monthly", Logme::TIME_ROTATION_MONTHLY, false },
    { "none", Logme::TIME_ROTATION_NONE, false },
    { "off", Logme::TIME_ROTATION_NONE, false },
    { "disabled", Logme::TIME_ROTATION_NONE, false },
  };

  for (const TestCase& testCase : cases)
  {
    Json::Value config = MakeFileConfig();
    config["rotation"] = testCase.Value;

    Logme::FileBackendConfig backendConfig;

    ASSERT_TRUE(backendConfig.Parse(&config)) << testCase.Value;
    EXPECT_EQ(backendConfig.TimeRotation, testCase.Mode) << testCase.Value;
    EXPECT_EQ(backendConfig.DailyRotation, testCase.DailyCompatibility) << testCase.Value;
  }
}

TEST(FileBackendConfigTest, RejectsUnknownTimeRotationMode)
{
  Json::Value config = MakeFileConfig();
  config["rotation"] = "century";

  Logme::FileBackendConfig backendConfig;

  EXPECT_FALSE(backendConfig.Parse(&config));
}

TEST(FileBackendConfigTest, RotateSizeLimitRequiresArchive)
{
  Json::Value config = MakeFileConfig();
  config["on-size-limit"] = "rotate";

  Logme::FileBackendConfig backendConfig;

  EXPECT_FALSE(backendConfig.Parse(&config));
}

TEST(FileBackendConfigTest, RotateSizeLimitRequiresArchiveIndex)
{
  Json::Value config = MakeFileConfig();
  config["on-size-limit"] = "rotate";
  config["archive"] = "archive.log";

  Logme::FileBackendConfig backendConfig;

  EXPECT_FALSE(backendConfig.Parse(&config));
}

TEST(FileBackendConfigTest, AcceptsRotateSizeLimitWithIndexedArchive)
{
  Json::Value config = MakeFileConfig();
  config["on-size-limit"] = "rotate";
  config["archive"] = "archive.{index}.log";

  Logme::FileBackendConfig backendConfig;

  ASSERT_TRUE(backendConfig.Parse(&config));
  EXPECT_EQ(backendConfig.OnSizeLimit, Logme::SIZE_LIMIT_ROTATE);
  EXPECT_EQ(backendConfig.ArchiveFilename, "archive.{index}.log");
}

TEST(FileBackendConfigTest, RejectsConflictingLegacyAndRetentionFileLimits)
{
  Json::Value config = MakeFileConfig();
  config["max-parts"] = 3;
  config["retention"]["max-files"] = 4;

  Logme::FileBackendConfig backendConfig;

  EXPECT_FALSE(backendConfig.Parse(&config));
}

TEST(FileBackendConfigTest, AcceptsRetentionRules)
{
  Json::Value config = MakeFileConfig();
  config["retention"]["max-files"] = 5;
  config["retention"]["max-age"] = "2d";
  config["retention"]["max-total-size"] = "128Kb";
  config["retention"]["clean-on-start"] = false;

  Logme::FileBackendConfig backendConfig;

  ASSERT_TRUE(backendConfig.Parse(&config));
  EXPECT_EQ(backendConfig.MaxParts, 5);
  EXPECT_EQ(backendConfig.RetentionMaxAge, 2ULL * 24ULL * 60ULL * 60ULL * 1000ULL);
  EXPECT_EQ(backendConfig.RetentionMaxTotalSize, 128ULL * 1024ULL);
  EXPECT_FALSE(backendConfig.RetentionCleanOnStart);
}
