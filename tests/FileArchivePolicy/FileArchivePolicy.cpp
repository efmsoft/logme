#include <gtest/gtest.h>

#include "../../logme/source/File/FileArchivePolicy.h"

#include <atomic>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>

namespace fs = std::filesystem;

namespace
{
  std::atomic<unsigned> Counter(0);

  fs::path MakeTestDirectory(const char* name)
  {
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    unsigned id = Counter.fetch_add(1, std::memory_order_relaxed);

    fs::path dir = fs::temp_directory_path()
      / (std::string("logme-file-archive-policy-")
      + name
      + "-"
      + std::to_string(now)
      + "-"
      + std::to_string(id));

    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir, ec);

    EXPECT_FALSE(ec);
    return dir;
  }

  std::time_t MakeLocalTime(
    int year
    , int month
    , int day
    , int hour = 0
    , int minute = 0
    , int second = 0
  )
  {
    struct tm tmValue {};
    tmValue.tm_year = year - 1900;
    tmValue.tm_mon = month - 1;
    tmValue.tm_mday = day;
    tmValue.tm_hour = hour;
    tmValue.tm_min = minute;
    tmValue.tm_sec = second;
    tmValue.tm_isdst = -1;

    return mktime(&tmValue);
  }

  std::string NativeString(const fs::path& path)
  {
    return path.string();
  }

  std::string GenericString(const fs::path& path)
  {
    return path.generic_string();
  }

  std::string NormalizeSeparators(std::string value)
  {
    for (char& ch : value)
    {
      if (ch == '\\')
        ch = '/';
    }

    return value;
  }

  void WriteFile(const fs::path& path)
  {
    fs::create_directories(path.parent_path());

    std::ofstream output(path, std::ios::binary);
    ASSERT_TRUE(output.good());
    output << "data";
    output.close();
    ASSERT_TRUE(output.good());
  }

  class FileArchivePolicyTest : public ::testing::Test
  {
  protected:
    fs::path Dir;

    void SetUp() override
    {
      Dir = MakeTestDirectory("case");
    }

    void TearDown() override
    {
      std::error_code ec;
      fs::remove_all(Dir, ec);
    }
  };
}

TEST_F(FileArchivePolicyTest, BuildNameUsesHomeDirectoryForRelativePattern)
{
  Logme::FileArchivePolicy policy;
  fs::path homePath = Dir;
  homePath /= "";
  const std::string home = NativeString(homePath);

  policy.Configure("archive/app.{index}.log", home, false);

  fs::path expected = Dir / "archive" / "app.7.log";
  EXPECT_EQ(
    NormalizeSeparators(policy.BuildName(7, MakeLocalTime(2026, 6, 17)))
    , GenericString(expected)
  );
}

TEST_F(FileArchivePolicyTest, BuildNameUsesProvidedDateAndIndex)
{
  Logme::FileArchivePolicy policy;
  fs::path archive = Dir / "app.{date}.{index}.log";

  policy.Configure(NativeString(archive), std::string(), false);

  fs::path expected = Dir / "app.2026-06-17.3.log";
  EXPECT_EQ(policy.BuildName(3, MakeLocalTime(2026, 6, 17)), NativeString(expected));
}

TEST_F(FileArchivePolicyTest, TakeNameSkipsExistingPlainAndGzipArchives)
{
  Logme::FileArchivePolicy policy;
  fs::path archive = Dir / "app.{index}.log";

  WriteFile(Dir / "app.1.log");
  WriteFile(Dir / "app.2.log.gz");

  policy.Configure(NativeString(archive), std::string(), true);

  fs::path expected = Dir / "app.3.log";
  EXPECT_EQ(policy.TakeName(MakeLocalTime(2026, 6, 17)), NativeString(expected));
}

TEST_F(FileArchivePolicyTest, StartupRecoveryContinuesIndexForCurrentDateOnly)
{
  Logme::FileArchivePolicy policy;
  fs::path archive = Dir / "app.{date}.{index}.log";

  WriteFile(Dir / "app.1999-01-01.99.log");
  WriteFile(Dir / "app.2026-06-17.1.log");

  policy.Configure(NativeString(archive), std::string(), false);

  fs::path expected = Dir / "app.2026-06-17.2.log";
  EXPECT_EQ(policy.TakeName(MakeLocalTime(2026, 6, 17, 10)), NativeString(expected));
}

TEST_F(FileArchivePolicyTest, BuildCleanPatternMatchesArchivePattern)
{
  Logme::FileArchivePolicy policy;
  fs::path archive = Dir / "app.{date}.{index}.log";

  policy.Configure(NativeString(archive), std::string(), true);

  std::regex pattern(policy.BuildCleanPattern());

  EXPECT_TRUE(std::regex_match(NativeString(Dir / "app.2026-06-17.1.log"), pattern));
  EXPECT_TRUE(std::regex_match(NativeString(Dir / "app.2026-06-18.25.log"), pattern));
  EXPECT_FALSE(std::regex_match(NativeString(Dir / "app.2026-06-17.1.log.gz"), pattern));
  EXPECT_FALSE(std::regex_match(NativeString(Dir / "app.log"), pattern));
}
