#include <gtest/gtest.h>

#include <Logme/File/RetentionCleaner.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>

namespace fs = std::filesystem;

namespace
{
  std::atomic<unsigned> Counter(0);

  std::string EscapeRegex(const std::string& value)
  {
    std::string result;
    result.reserve(value.size() * 2);

    for (char ch : value)
    {
      switch (ch)
      {
        case '\\':
        case '^':
        case '$':
        case '.':
        case '|':
        case '?':
        case '*':
        case '+':
        case '(':
        case ')':
        case '[':
        case ']':
        case '{':
        case '}':
          result.push_back('\\');
          break;

        default:
          break;
      }

      result.push_back(ch);
    }

    return result;
  }

  fs::path MakeTestDirectory()
  {
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    unsigned id = Counter.fetch_add(1, std::memory_order_relaxed);

    fs::path dir = fs::temp_directory_path()
      / ("logme-retention-cleaner-test-" + std::to_string(now) + "-" + std::to_string(id));

    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir, ec);

    EXPECT_FALSE(ec);
    return dir;
  }

  void WriteFile(
    const fs::path& file
    , std::size_t size
    , std::chrono::hours age
  )
  {
    std::ofstream output(file, std::ios::binary);
    ASSERT_TRUE(output.good());

    for (std::size_t i = 0; i < size; ++i)
      output.put(static_cast<char>('a' + (i % 26)));

    output.close();
    ASSERT_TRUE(output.good());

    std::error_code ec;
    fs::last_write_time(
      file
      , fs::file_time_type::clock::now() - age
      , ec
    );

    ASSERT_FALSE(ec);
  }

  std::regex MakeLogPattern(const fs::path& dir)
  {
    std::string pattern = EscapeRegex(dir.string());
    pattern += R"([/\\][^/\\]+\.log(\.gz)?)";
    return std::regex(pattern);
  }

  class RetentionCleanerTest : public ::testing::Test
  {
  protected:
    fs::path Dir;

    void SetUp() override
    {
      Dir = MakeTestDirectory();
    }

    void TearDown() override
    {
      std::error_code ec;
      fs::remove_all(Dir, ec);
    }
  };
}

TEST_F(RetentionCleanerTest, MaxFilesKeepsNewestFiles)
{
  fs::path keep = Dir / "current.log";
  fs::path old1 = Dir / "old1.log";
  fs::path old2 = Dir / "old2.log";
  fs::path newest = Dir / "newest.log";

  WriteFile(keep, 1, std::chrono::hours(0));
  WriteFile(old1, 1, std::chrono::hours(72));
  WriteFile(old2, 1, std::chrono::hours(48));
  WriteFile(newest, 1, std::chrono::hours(24));

  Logme::RetentionOptions options;
  options.MaxFiles = 2;

  Logme::CleanFiles(MakeLogPattern(Dir), keep.string(), options);

  EXPECT_TRUE(fs::exists(keep));
  EXPECT_FALSE(fs::exists(old1));
  EXPECT_FALSE(fs::exists(old2));
  EXPECT_TRUE(fs::exists(newest));
}

TEST_F(RetentionCleanerTest, MaxFilesDoesNotDeleteKeepFileWhenKeepIsOld)
{
  fs::path keep = Dir / "current.log";
  fs::path old1 = Dir / "old1.log";
  fs::path old2 = Dir / "old2.log";
  fs::path newest = Dir / "newest.log";

  WriteFile(keep, 1, std::chrono::hours(96));
  WriteFile(old1, 1, std::chrono::hours(72));
  WriteFile(old2, 1, std::chrono::hours(48));
  WriteFile(newest, 1, std::chrono::hours(24));

  Logme::RetentionOptions options;
  options.MaxFiles = 2;

  Logme::CleanFiles(MakeLogPattern(Dir), keep.string(), options);

  EXPECT_TRUE(fs::exists(keep));
  EXPECT_FALSE(fs::exists(old1));
  EXPECT_FALSE(fs::exists(old2));
  EXPECT_TRUE(fs::exists(newest));
}

TEST_F(RetentionCleanerTest, MaxAgeDeletesOldFiles)
{
  fs::path keep = Dir / "current.log";
  fs::path oldFile = Dir / "old.log";
  fs::path recent = Dir / "recent.log";

  WriteFile(keep, 1, std::chrono::hours(0));
  WriteFile(oldFile, 1, std::chrono::hours(72));
  WriteFile(recent, 1, std::chrono::hours(1));

  Logme::RetentionOptions options;
  options.MaxAgeMs = 24ULL * 60ULL * 60ULL * 1000ULL;

  Logme::CleanFiles(MakeLogPattern(Dir), keep.string(), options);

  EXPECT_TRUE(fs::exists(keep));
  EXPECT_FALSE(fs::exists(oldFile));
  EXPECT_TRUE(fs::exists(recent));
}

TEST_F(RetentionCleanerTest, MaxTotalSizeDeletesOldestUntilBelowLimit)
{
  fs::path keep = Dir / "current.log";
  fs::path old1 = Dir / "old1.log";
  fs::path old2 = Dir / "old2.log";
  fs::path newest = Dir / "newest.log";

  WriteFile(keep, 3, std::chrono::hours(0));
  WriteFile(old1, 4, std::chrono::hours(72));
  WriteFile(old2, 4, std::chrono::hours(48));
  WriteFile(newest, 4, std::chrono::hours(24));

  Logme::RetentionOptions options;
  options.MaxTotalSize = 9;

  Logme::CleanFiles(MakeLogPattern(Dir), keep.string(), options);

  EXPECT_TRUE(fs::exists(keep));
  EXPECT_FALSE(fs::exists(old1));
  EXPECT_FALSE(fs::exists(old2));
  EXPECT_TRUE(fs::exists(newest));
}

TEST_F(RetentionCleanerTest, ZeroLimitsDisableRules)
{
  fs::path keep = Dir / "current.log";
  fs::path oldFile = Dir / "old.log";

  WriteFile(keep, 1, std::chrono::hours(0));
  WriteFile(oldFile, 1, std::chrono::hours(72));

  Logme::RetentionOptions options;

  Logme::CleanFiles(MakeLogPattern(Dir), keep.string(), options);

  EXPECT_TRUE(fs::exists(keep));
  EXPECT_TRUE(fs::exists(oldFile));
}

TEST_F(RetentionCleanerTest, CompressedFilesAreMatchedWhenPatternAllowsGzip)
{
  fs::path keep = Dir / "current.log";
  fs::path oldLog = Dir / "old.log";
  fs::path oldGzip = Dir / "old-gzip.log.gz";

  WriteFile(keep, 1, std::chrono::hours(0));
  WriteFile(oldLog, 1, std::chrono::hours(72));
  WriteFile(oldGzip, 1, std::chrono::hours(48));

  Logme::RetentionOptions options;
  options.MaxFiles = 1;

  Logme::CleanFiles(MakeLogPattern(Dir), keep.string(), options);

  EXPECT_TRUE(fs::exists(keep));
  EXPECT_FALSE(fs::exists(oldLog));
  EXPECT_FALSE(fs::exists(oldGzip));
}

TEST(RetentionCleaner, MissingDirectoryDoesNotFail)
{
  fs::path dir = fs::temp_directory_path() / "logme-retention-cleaner-missing-directory";
  fs::path keep = dir / "current.log";

  std::error_code ec;
  fs::remove_all(dir, ec);

  Logme::RetentionOptions options;
  options.MaxFiles = 1;

  EXPECT_NO_THROW(Logme::CleanFiles(MakeLogPattern(dir), keep.string(), options));
}
