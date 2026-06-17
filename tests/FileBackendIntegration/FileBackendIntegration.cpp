#include <gtest/gtest.h>

#include <Logme/Backend/FileBackend.h>
#include <Logme/Channel.h>
#include <Logme/Logme.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <thread>

namespace fs = std::filesystem;

namespace
{
  std::atomic<unsigned> Counter(0);

  fs::path MakeTestDirectory(const char* name)
  {
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    unsigned id = Counter.fetch_add(1, std::memory_order_relaxed);

    fs::path dir = fs::temp_directory_path()
      / (std::string("logme-file-backend-integration-")
      + name
      + "-"
      + std::to_string(now)
      + "-"
      + std::to_string(id));

    std::error_code ec;
    fs::remove_all(dir, ec);
    ec.clear();
    fs::create_directories(dir, ec);

    EXPECT_FALSE(ec);
    return dir;
  }

  std::string ReadFile(const fs::path& file)
  {
    std::ifstream input(file, std::ios::binary);
    return std::string(
      std::istreambuf_iterator<char>(input)
      , std::istreambuf_iterator<char>()
    );
  }

  void WriteFile(
    const fs::path& file
    , const std::string& text
    , std::chrono::hours age = std::chrono::hours(0)
  )
  {
    fs::create_directories(file.parent_path());

    std::ofstream output(file, std::ios::binary);
    ASSERT_TRUE(output.good());
    output.write(text.data(), static_cast<std::streamsize>(text.size()));
    ASSERT_TRUE(output.good());
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

#ifdef LOGME_TEST_USE_ZLIB
  bool WaitForCompressedFile(
    const fs::path& source
    , const fs::path& compressed
    , std::chrono::milliseconds timeout
  )
  {
    auto deadline = std::chrono::steady_clock::now() + timeout;

    while (std::chrono::steady_clock::now() < deadline)
    {
      std::error_code ec;
      bool compressedExists = fs::exists(compressed, ec) && !ec;
      ec.clear();
      bool sourceExists = fs::exists(source, ec) && !ec;

      if (compressedExists && !sourceExists)
        return true;

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::error_code ec;
    bool compressedExists = fs::exists(compressed, ec) && !ec;
    ec.clear();
    bool sourceExists = fs::exists(source, ec) && !ec;
    return compressedExists && !sourceExists;
  }
#endif

  class FileBackendIntegrationTest : public ::testing::Test
  {
  protected:
    fs::path Dir;
    fs::path ArchiveDir;
    fs::path Active;
    fs::path ActiveGz;
    fs::path Archive1;
    fs::path Archive2;
    fs::path Archive3;
    fs::path Archive1Gz;
    fs::path Archive2Gz;
    std::string ArchivePattern;
    std::string ChannelName;
    Logme::ChannelPtr Channel;
    std::shared_ptr<Logme::FileBackend> Backend;
    bool BackendAdded = false;

    void SetUp() override
    {
      Dir = MakeTestDirectory("case");
      ArchiveDir = Dir / "archive";
      Active = Dir / "active.log";
      ActiveGz = Dir / "active.log.gz";
      Archive1 = ArchiveDir / "app.1.log";
      Archive2 = ArchiveDir / "app.2.log";
      Archive3 = ArchiveDir / "app.3.log";
      Archive1Gz = ArchiveDir / "app.1.log.gz";
      Archive2Gz = ArchiveDir / "app.2.log.gz";
      ArchivePattern = (ArchiveDir / "app.{index}.log").string();

      ChannelName = "file-backend-integration-" + std::to_string(
        Counter.fetch_add(1, std::memory_order_relaxed)
      );

      ASSERT_TRUE(Logme::Instance != nullptr);

      Logme::OutputFlags flags;
      flags.Value = 0;

      Logme::ID channelId{ChannelName.c_str()};
      Channel = Logme::Instance->CreateChannel(
        channelId
        , flags
        , Logme::LEVEL_DEBUG
      );

      ASSERT_TRUE(Channel != nullptr);
      Channel->RemoveBackends();
      Channel->SetFlags(flags);
      Channel->SetFilterLevel(Logme::LEVEL_DEBUG);

      Backend = std::make_shared<Logme::FileBackend>(Channel);
      ASSERT_TRUE(Backend != nullptr);
    }

    void TearDown() override
    {
      if (Backend)
      {
        Backend->Flush();
      }

      if (Channel)
      {
        Channel->RemoveBackends();
      }

      Backend.reset();
      Channel.reset();

      std::error_code ec;
      fs::remove_all(Dir, ec);
    }

    std::shared_ptr<Logme::FileBackendConfig> MakeConfig(
      Logme::SizeLimitPolicy sizeLimitPolicy
      , std::size_t maxSize
    )
    {
      auto config = std::make_shared<Logme::FileBackendConfig>();
      config->Async = false;
      config->Append = false;
      config->MaxSize = maxSize;
      config->Filename = Active.string();
      config->OnSizeLimit = sizeLimitPolicy;
      config->MaxParts = 0;
      config->RetentionMaxAge = 0;
      config->RetentionMaxTotalSize = 0;
      config->RetentionCleanOnStart = true;
      config->GzipCompression = false;

      if (sizeLimitPolicy == Logme::SIZE_LIMIT_ROTATE)
        config->ArchiveFilename = ArchivePattern;

      return config;
    }

    void ApplyConfig(const std::shared_ptr<Logme::FileBackendConfig>& config)
    {
      ASSERT_TRUE(Backend->ApplyConfig(config));

      if (!BackendAdded)
      {
        Channel->AddBackend(Backend);
        BackendAdded = true;
      }
    }

    void Write(const std::string& text)
    {
      Backend->AppendString(text.data(), text.size());
      Backend->Flush();
    }
  };
}

TEST_F(FileBackendIntegrationTest, DefaultSizeLimitKeepsTruncateBehavior)
{
  auto config = MakeConfig(Logme::SIZE_LIMIT_TRUNCATE, 2048);
  ApplyConfig(config);

  Write(std::string(1500, 'A'));
  Write(std::string(1500, 'B'));
  Write("tail-marker");

  ASSERT_TRUE(fs::exists(Active));
  EXPECT_FALSE(fs::exists(Archive1));
  EXPECT_FALSE(fs::exists(Archive2));
  EXPECT_NE(ReadFile(Active).find("tail-marker"), std::string::npos);
}

TEST_F(FileBackendIntegrationTest, ExplicitTruncatePolicyDoesNotCreateArchives)
{
  auto config = MakeConfig(Logme::SIZE_LIMIT_TRUNCATE, 2048);
  config->ArchiveFilename = ArchivePattern;
  ApplyConfig(config);

  Write(std::string(1500, 'A'));
  Write(std::string(1500, 'B'));
  Write(std::string(1500, 'C'));

  ASSERT_TRUE(fs::exists(Active));
  EXPECT_FALSE(fs::exists(Archive1));
  EXPECT_FALSE(fs::exists(Archive2));
}

TEST_F(FileBackendIntegrationTest, RotatePolicyCreatesArchiveAndKeepsActiveWritable)
{
  auto config = MakeConfig(Logme::SIZE_LIMIT_ROTATE, 2048);
  ApplyConfig(config);

  const std::string first(1500, 'A');
  const std::string second(1500, 'B');
  const std::string tail("tail-after-rotate");

  Write(first);
  Write(second);
  Write(tail);

  ASSERT_TRUE(fs::exists(Archive1));
  ASSERT_TRUE(fs::exists(Active));
  EXPECT_EQ(ReadFile(Archive1), first);
  EXPECT_NE(ReadFile(Active).find(second), std::string::npos);
  EXPECT_NE(ReadFile(Active).find(tail), std::string::npos);
}

TEST_F(FileBackendIntegrationTest, StartupRecoverySkipsExistingPlainAndGzipArchives)
{
  WriteFile(Archive1, "existing-one");
  WriteFile(Archive2Gz, "existing-two-gzip-name");

  auto config = MakeConfig(Logme::SIZE_LIMIT_ROTATE, 2048);
  ApplyConfig(config);

  const std::string first(1500, 'A');
  const std::string second(1500, 'B');

  Write(first);
  Write(second);

  EXPECT_EQ(ReadFile(Archive1), "existing-one");
  EXPECT_TRUE(fs::exists(Archive2Gz));
  EXPECT_FALSE(fs::exists(Archive2));
  ASSERT_TRUE(fs::exists(Archive3));
  EXPECT_EQ(ReadFile(Archive3), first);
  EXPECT_EQ(ReadFile(Active), second);
}

TEST_F(FileBackendIntegrationTest, RetentionMaxFilesAppliesToArchiveDirectory)
{
  auto config = MakeConfig(Logme::SIZE_LIMIT_ROTATE, 2048);
  config->MaxParts = 2;
  ApplyConfig(config);

  const std::string first(1500, 'A');
  const std::string second(1500, 'B');
  const std::string third(1500, 'C');

  Write(first);
  Write(second);
  Write(third);

  ASSERT_TRUE(fs::exists(Active));
  EXPECT_FALSE(fs::exists(Archive1));
  ASSERT_TRUE(fs::exists(Archive2));
  EXPECT_EQ(ReadFile(Archive2), second);
  EXPECT_EQ(ReadFile(Active), third);
}

TEST_F(FileBackendIntegrationTest, RetentionMaxAgeCleansOldArchivesOnStart)
{
  WriteFile(Archive1, "old", std::chrono::hours(72));
  WriteFile(Archive2, "recent", std::chrono::hours(1));

  auto config = MakeConfig(Logme::SIZE_LIMIT_ROTATE, 2048);
  config->RetentionMaxAge = 24ULL * 60ULL * 60ULL * 1000ULL;
  ApplyConfig(config);

  ASSERT_TRUE(fs::exists(Active));
  EXPECT_FALSE(fs::exists(Archive1));
  ASSERT_TRUE(fs::exists(Archive2));
  EXPECT_EQ(ReadFile(Archive2), "recent");
}

TEST_F(FileBackendIntegrationTest, RetentionMaxTotalSizeCleansOldestArchivesOnStart)
{
  WriteFile(Archive1, std::string(4096, 'A'), std::chrono::hours(72));
  WriteFile(Archive2, std::string(4096, 'B'), std::chrono::hours(48));
  WriteFile(Archive3, std::string(512, 'C'), std::chrono::hours(1));

  auto config = MakeConfig(Logme::SIZE_LIMIT_ROTATE, 2048);
  config->RetentionMaxTotalSize = 2048;
  ApplyConfig(config);

  ASSERT_TRUE(fs::exists(Active));
  EXPECT_FALSE(fs::exists(Archive1));
  EXPECT_FALSE(fs::exists(Archive2));
  ASSERT_TRUE(fs::exists(Archive3));
  EXPECT_EQ(ReadFile(Archive3), std::string(512, 'C'));
}

#ifdef LOGME_TEST_USE_ZLIB
TEST_F(FileBackendIntegrationTest, GzipCompressionAppliesOnlyToCompletedArchive)
{
  auto config = MakeConfig(Logme::SIZE_LIMIT_ROTATE, 2048);
  config->GzipCompression = true;
  ApplyConfig(config);

  const std::string first(1500, 'A');
  const std::string second(1500, 'B');

  Write(first);
  Write(second);

  ASSERT_TRUE(WaitForCompressedFile(Archive1, Archive1Gz, std::chrono::seconds(5)));
  EXPECT_FALSE(fs::exists(Archive1));
  ASSERT_TRUE(fs::exists(Active));
  EXPECT_FALSE(fs::exists(ActiveGz));
  EXPECT_EQ(ReadFile(Active), second);
}
#endif
