#include <gtest/gtest.h>

#include <Logme/Backend/FileBackend.h>
#include <Logme/Logme.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>

namespace
{
  std::atomic<unsigned> FileIndex(0);

  std::filesystem::path MakeBasePath(const char* name)
  {
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    auto index = FileIndex.fetch_add(1, std::memory_order_relaxed);
    auto file = std::string("logme-size-limit-")
      + name
      + "-"
      + std::to_string(now)
      + "-"
      + std::to_string(index);

    return std::filesystem::temp_directory_path() / file;
  }

  std::string ReadFile(const std::filesystem::path& path)
  {
    std::ifstream input(path, std::ios::binary);
    return std::string(
      std::istreambuf_iterator<char>(input)
      , std::istreambuf_iterator<char>()
    );
  }

  void WriteFile(
    const std::filesystem::path& path
    , const std::string& text
    , std::chrono::hours age = std::chrono::hours(0)
  )
  {
    std::ofstream output(path, std::ios::binary);
    ASSERT_TRUE(output.good());
    output << text;
    output.close();
    ASSERT_TRUE(output.good());

    std::error_code ec;
    std::filesystem::last_write_time(
      path
      , std::filesystem::file_time_type::clock::now() - age
      , ec
    );
    ASSERT_FALSE(ec);
  }

  void RemoveIfExists(const std::filesystem::path& path)
  {
    std::error_code ec;
    std::filesystem::remove(path, ec);
  }

  struct FileFixture
  {
    std::string ChannelName;
    Logme::ID ChannelId;
    Logme::ChannelPtr Channel;
    std::shared_ptr<Logme::FileBackend> Backend;
    std::filesystem::path Base;
    std::filesystem::path Active;
    std::filesystem::path Archive1;
    std::filesystem::path Archive2;
    std::filesystem::path Archive3;
    std::filesystem::path Archive1Gz;
    std::string ArchivePattern;

    FileFixture(
      const char* name
      , Logme::SizeLimitPolicy policy
      , int maxParts
      , bool cleanOnStart = true
      , bool applyConfig = true
    )
      : ChannelName(name)
      , ChannelId{ChannelName.c_str()}
      , Base(MakeBasePath(name))
      , Active(Base.string() + ".log")
      , Archive1(Base.string() + ".1.log")
      , Archive2(Base.string() + ".2.log")
      , Archive3(Base.string() + ".3.log")
      , Archive1Gz(Base.string() + ".1.log.gz")
      , ArchivePattern(Base.string() + ".{index}.log")
    {
      RemoveIfExists(Active);
      RemoveIfExists(Archive1);
      RemoveIfExists(Archive2);
      RemoveIfExists(Archive3);
      RemoveIfExists(Archive1Gz);

      Logme::OutputFlags flags;
      flags.Value = 0;

      Channel = Logme::Instance->CreateChannel(
        ChannelId
        , flags
        , Logme::LEVEL_DEBUG
      );
      Channel->RemoveBackends();
      Channel->SetFlags(flags);
      Channel->SetFilterLevel(Logme::LEVEL_DEBUG);

      Backend = std::make_shared<Logme::FileBackend>(Channel);

      if (applyConfig)
        ApplyConfig(policy, maxParts, cleanOnStart);
    }

    ~FileFixture()
    {
      if (Backend)
        Backend->Flush();

      if (Channel)
        Channel->RemoveBackends();

      RemoveIfExists(Active);
      RemoveIfExists(Archive1);
      RemoveIfExists(Archive2);
      RemoveIfExists(Archive3);
      RemoveIfExists(Archive1Gz);
    }

    void ApplyConfig(
      Logme::SizeLimitPolicy policy
      , int maxParts
      , bool cleanOnStart = true
    )
    {
      auto config = std::make_shared<Logme::FileBackendConfig>();
      config->Async = false;
      config->Append = false;
      config->MaxSize = 2048;
      config->Filename = Active.string();
      config->OnSizeLimit = policy;
      config->MaxParts = maxParts;
      config->RetentionCleanOnStart = cleanOnStart;
      if (policy == Logme::SIZE_LIMIT_ROTATE)
        config->ArchiveFilename = ArchivePattern;

      EXPECT_TRUE(Backend->ApplyConfig(config));
      Channel->AddBackend(Backend);
    }

    void Write(const std::string& text)
    {
      Backend->AppendString(text.c_str(), text.size());
      Backend->Flush();
    }
  };
}

TEST(SizeLimitPolicy, DefaultPolicyKeepsTruncateBehavior)
{
  FileFixture fixture(
    "truncate-default"
    , Logme::SIZE_LIMIT_TRUNCATE
    , 0
  );

  fixture.Write(std::string(1500, 'A'));
  fixture.Write(std::string(1500, 'B'));
  fixture.Write(std::string(1500, 'C'));

  EXPECT_TRUE(std::filesystem::exists(fixture.Active));
  EXPECT_FALSE(std::filesystem::exists(fixture.Archive1));
  EXPECT_FALSE(std::filesystem::exists(fixture.Archive2));
}

TEST(SizeLimitPolicy, RotatePolicyCreatesArchiveAndReopensActiveFile)
{
  FileFixture fixture(
    "rotate"
    , Logme::SIZE_LIMIT_ROTATE
    , 0
  );

  const std::string first(1500, 'A');
  const std::string second(1500, 'B');

  fixture.Write(first);
  fixture.Write(second);

  ASSERT_TRUE(std::filesystem::exists(fixture.Active));
  ASSERT_TRUE(std::filesystem::exists(fixture.Archive1));

  EXPECT_EQ(ReadFile(fixture.Archive1), first);
  EXPECT_EQ(ReadFile(fixture.Active), second);
}

TEST(SizeLimitPolicy, RotatePolicyAppliesMaxFilesRetention)
{
  FileFixture fixture(
    "rotate-retention"
    , Logme::SIZE_LIMIT_ROTATE
    , 2
  );

  const std::string first(1500, 'A');
  const std::string second(1500, 'B');
  const std::string third(1500, 'C');

  fixture.Write(first);
  fixture.Write(second);
  fixture.Write(third);

  EXPECT_TRUE(std::filesystem::exists(fixture.Active));
  EXPECT_FALSE(std::filesystem::exists(fixture.Archive1));
  ASSERT_TRUE(std::filesystem::exists(fixture.Archive2));
  EXPECT_EQ(ReadFile(fixture.Archive2), second);
  EXPECT_EQ(ReadFile(fixture.Active), third);
}

TEST(SizeLimitPolicy, RotatePolicyContinuesArchiveIndexAfterRestart)
{
  FileFixture fixture(
    "rotate-restart"
    , Logme::SIZE_LIMIT_ROTATE
    , 0
    , true
    , false
  );

  WriteFile(fixture.Archive1, "existing-one", std::chrono::hours(2));
  WriteFile(fixture.Archive2, "existing-two", std::chrono::hours(1));

  fixture.ApplyConfig(Logme::SIZE_LIMIT_ROTATE, 0);

  const std::string first(1500, 'A');
  const std::string second(1500, 'B');

  fixture.Write(first);
  fixture.Write(second);

  EXPECT_EQ(ReadFile(fixture.Archive1), "existing-one");
  EXPECT_EQ(ReadFile(fixture.Archive2), "existing-two");
  ASSERT_TRUE(std::filesystem::exists(fixture.Archive3));
  EXPECT_EQ(ReadFile(fixture.Archive3), first);
  EXPECT_EQ(ReadFile(fixture.Active), second);
}

TEST(SizeLimitPolicy, RotatePolicySkipsExistingCompressedArchiveName)
{
  FileFixture fixture(
    "rotate-skip-gzip"
    , Logme::SIZE_LIMIT_ROTATE
    , 0
    , true
    , false
  );

  WriteFile(fixture.Archive1Gz, "existing-gzip-name");

  fixture.ApplyConfig(Logme::SIZE_LIMIT_ROTATE, 0);

  const std::string first(1500, 'A');
  const std::string second(1500, 'B');

  fixture.Write(first);
  fixture.Write(second);

  EXPECT_TRUE(std::filesystem::exists(fixture.Archive1Gz));
  EXPECT_FALSE(std::filesystem::exists(fixture.Archive1));
  ASSERT_TRUE(std::filesystem::exists(fixture.Archive2));
  EXPECT_EQ(ReadFile(fixture.Archive2), first);
}

TEST(SizeLimitPolicy, CleanOnStartAppliesMaxFilesRetention)
{
  FileFixture fixture(
    "clean-on-start"
    , Logme::SIZE_LIMIT_ROTATE
    , 2
    , true
    , false
  );

  WriteFile(fixture.Archive1, "old", std::chrono::hours(2));
  WriteFile(fixture.Archive2, "new", std::chrono::hours(1));

  fixture.ApplyConfig(Logme::SIZE_LIMIT_ROTATE, 2, true);

  EXPECT_FALSE(std::filesystem::exists(fixture.Archive1));
  ASSERT_TRUE(std::filesystem::exists(fixture.Archive2));
  EXPECT_EQ(ReadFile(fixture.Archive2), "new");
  EXPECT_TRUE(std::filesystem::exists(fixture.Active));
}

TEST(SizeLimitPolicy, CleanOnStartCanBeDisabled)
{
  FileFixture fixture(
    "clean-on-start-disabled"
    , Logme::SIZE_LIMIT_ROTATE
    , 2
    , false
    , false
  );

  WriteFile(fixture.Archive1, "old", std::chrono::hours(2));
  WriteFile(fixture.Archive2, "new", std::chrono::hours(1));

  fixture.ApplyConfig(Logme::SIZE_LIMIT_ROTATE, 2, false);

  EXPECT_TRUE(std::filesystem::exists(fixture.Archive1));
  EXPECT_TRUE(std::filesystem::exists(fixture.Archive2));
  EXPECT_TRUE(std::filesystem::exists(fixture.Active));
}
