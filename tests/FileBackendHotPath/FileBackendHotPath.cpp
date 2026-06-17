#include <gtest/gtest.h>

#include <Logme/Backend/FileBackend.h>
#include <Logme/Channel.h>
#include <Logme/Logme.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

namespace fs = std::filesystem;

namespace
{
  constexpr std::size_t HOT_PATH_ITERATIONS = 4096;
  constexpr std::size_t HOT_PATH_RECORD_SIZE = 64;
  constexpr std::size_t HOT_PATH_MAX_SIZE = 1024 * 1024;

  std::atomic<unsigned> Counter(0);

  struct CounterDelta
  {
    std::uint64_t AppendCalls;
    std::uint64_t InputBytes;
    std::uint64_t ChangePartCalls;
    std::uint64_t ChangePartFailures;
    std::uint64_t SizeLimitCompletionCalls;
    std::uint64_t TimeLimitCompletionCalls;
    std::uint64_t ArchivedFiles;
    std::uint64_t CompressionSubmitCalls;
    std::uint64_t RetentionRuns;
    std::uint64_t WriteErrors;
  };

  fs::path MakeTestDirectory(const char* name)
  {
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    unsigned id = Counter.fetch_add(1, std::memory_order_relaxed);

    fs::path dir = fs::temp_directory_path()
      / (std::string("logme-file-backend-hot-path-")
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

  CounterDelta DiffCounters(
    const Logme::FileBackendCounters& before
    , const Logme::FileBackendCounters& after
  )
  {
    CounterDelta delta{};
    delta.AppendCalls = after.AppendCalls - before.AppendCalls;
    delta.InputBytes = after.InputBytes - before.InputBytes;
    delta.ChangePartCalls = after.ChangePartCalls - before.ChangePartCalls;
    delta.ChangePartFailures = after.ChangePartFailures - before.ChangePartFailures;
    delta.SizeLimitCompletionCalls =
      after.SizeLimitCompletionCalls - before.SizeLimitCompletionCalls;
    delta.TimeLimitCompletionCalls =
      after.TimeLimitCompletionCalls - before.TimeLimitCompletionCalls;
    delta.ArchivedFiles = after.ArchivedFiles - before.ArchivedFiles;
    delta.CompressionSubmitCalls = after.CompressionSubmitCalls - before.CompressionSubmitCalls;
    delta.RetentionRuns = after.RetentionRuns - before.RetentionRuns;
    delta.WriteErrors = after.WriteErrors - before.WriteErrors;
    return delta;
  }


  std::uintmax_t FileSize(const fs::path& file)
  {
    std::error_code ec;
    std::uintmax_t size = fs::file_size(file, ec);

    EXPECT_FALSE(ec);
    if (ec)
      return 0;

    return size;
  }

  double NsPerCall(
    std::chrono::steady_clock::duration duration
    , std::size_t iterations
  )
  {
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    return static_cast<double>(ns) / static_cast<double>(iterations);
  }

  void PrintBenchmark(
    const char* name
    , double nsPerCall
    , const CounterDelta& delta
  )
  {
    std::cout
      << "[ HOTPATH ] "
      << name
      << ": "
      << nsPerCall
      << " ns/call"
      << ", append="
      << delta.AppendCalls
      << ", bytes="
      << delta.InputBytes
      << ", completions="
      << delta.ChangePartCalls
      << ", completion_failures="
      << delta.ChangePartFailures
      << ", size_completion="
      << delta.SizeLimitCompletionCalls
      << ", time_completion="
      << delta.TimeLimitCompletionCalls
      << ", archived="
      << delta.ArchivedFiles
      << ", compression_submits="
      << delta.CompressionSubmitCalls
      << ", retention_runs="
      << delta.RetentionRuns
      << ", write_errors="
      << delta.WriteErrors
      << std::endl;
  }

  class FileBackendHotPathTest : public ::testing::Test
  {
  protected:
    fs::path Dir;
    fs::path ArchiveDir;
    fs::path Active;
    fs::path Archive1;
    fs::path Archive2;
    std::string ArchivePattern;
    std::string ChannelName;
    Logme::ID ChannelId;
    Logme::ChannelPtr Channel;
    std::shared_ptr<Logme::FileBackend> Backend;
    bool BackendAdded = false;

    FileBackendHotPathTest()
      : ChannelId{nullptr}
    {
    }

    void SetUp() override
    {
      Dir = MakeTestDirectory("case");
      ArchiveDir = Dir / "archive";
      Active = Dir / "active.log";
      Archive1 = ArchiveDir / "app.1.log";
      Archive2 = ArchiveDir / "app.2.log";
      ArchivePattern = (ArchiveDir / "app.{index}.log").string();

      ChannelName = "file-backend-hot-path-" + std::to_string(
        Counter.fetch_add(1, std::memory_order_relaxed)
      );
      ChannelId = Logme::ID{ChannelName.c_str()};

      ASSERT_TRUE(Logme::Instance != nullptr);

      Logme::OutputFlags flags;
      flags.Value = 0;

      Channel = Logme::Instance->CreateChannel(
        ChannelId
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
      config->ArchiveFilename.clear();
      config->OnSizeLimit = sizeLimitPolicy;
      config->DailyRotation = false;
      config->TimeRotation = Logme::TIME_ROTATION_NONE;
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

    void AppendRecords(
      const std::string& payload
      , std::size_t iterations
    )
    {
      for (std::size_t i = 0; i < iterations; ++i)
      {
        Backend->AppendString(payload.data(), payload.size());
      }

      Backend->Flush();
    }

    void LogRecords(
      const std::string& payload
      , std::size_t iterations
    )
    {
      for (std::size_t i = 0; i < iterations; ++i)
      {
        LogmeI(ChannelId, "hotpath %s", payload.c_str());
      }

      Backend->Flush();
    }
  };
}

TEST_F(FileBackendHotPathTest, PlainAppendDoesNotCompleteFile)
{
  auto config = MakeConfig(Logme::SIZE_LIMIT_TRUNCATE, 0);
  ApplyConfig(config);

  const std::string payload(HOT_PATH_RECORD_SIZE, 'P');
  const auto before = Logme::FileBackend::GetCounters();
  const auto started = std::chrono::steady_clock::now();

  AppendRecords(payload, HOT_PATH_ITERATIONS);

  const auto elapsed = std::chrono::steady_clock::now() - started;
  const auto after = Logme::FileBackend::GetCounters();
  const CounterDelta delta = DiffCounters(before, after);

  PrintBenchmark("plain append", NsPerCall(elapsed, HOT_PATH_ITERATIONS), delta);

  EXPECT_EQ(delta.AppendCalls, HOT_PATH_ITERATIONS);
  EXPECT_EQ(delta.InputBytes, HOT_PATH_ITERATIONS * payload.size());
  EXPECT_EQ(delta.ChangePartCalls, 0u);
  EXPECT_EQ(delta.ChangePartFailures, 0u);
  EXPECT_EQ(delta.SizeLimitCompletionCalls, 0u);
  EXPECT_EQ(delta.TimeLimitCompletionCalls, 0u);
  EXPECT_EQ(delta.ArchivedFiles, 0u);
  EXPECT_EQ(delta.CompressionSubmitCalls, 0u);
  EXPECT_EQ(delta.RetentionRuns, 0u);
  EXPECT_EQ(delta.WriteErrors, 0u);
  EXPECT_FALSE(fs::exists(Archive1));
  EXPECT_TRUE(FileSize(Active) >= static_cast<std::uintmax_t>(HOT_PATH_ITERATIONS * payload.size()));
}

TEST_F(FileBackendHotPathTest, RotatePolicyBelowSizeLimitDoesNotCompleteFile)
{
  auto config = MakeConfig(Logme::SIZE_LIMIT_ROTATE, HOT_PATH_MAX_SIZE);
  ApplyConfig(config);

  const std::string payload(HOT_PATH_RECORD_SIZE, 'R');
  const auto before = Logme::FileBackend::GetCounters();
  const auto started = std::chrono::steady_clock::now();

  AppendRecords(payload, HOT_PATH_ITERATIONS);

  const auto elapsed = std::chrono::steady_clock::now() - started;
  const auto after = Logme::FileBackend::GetCounters();
  const CounterDelta delta = DiffCounters(before, after);

  PrintBenchmark("size rotate below limit", NsPerCall(elapsed, HOT_PATH_ITERATIONS), delta);

  EXPECT_EQ(delta.AppendCalls, HOT_PATH_ITERATIONS);
  EXPECT_EQ(delta.InputBytes, HOT_PATH_ITERATIONS * payload.size());
  EXPECT_EQ(delta.ChangePartCalls, 0u);
  EXPECT_EQ(delta.ChangePartFailures, 0u);
  EXPECT_EQ(delta.SizeLimitCompletionCalls, 0u);
  EXPECT_EQ(delta.TimeLimitCompletionCalls, 0u);
  EXPECT_EQ(delta.ArchivedFiles, 0u);
  EXPECT_EQ(delta.CompressionSubmitCalls, 0u);
  EXPECT_EQ(delta.RetentionRuns, 0u);
  EXPECT_EQ(delta.WriteErrors, 0u);
  EXPECT_FALSE(fs::exists(Archive1));
  EXPECT_FALSE(fs::exists(Archive2));
}

TEST_F(FileBackendHotPathTest, DailyRotationWithinCurrentPeriodDoesNotCompleteFile)
{
  auto config = MakeConfig(Logme::SIZE_LIMIT_ROTATE, HOT_PATH_MAX_SIZE);
  config->TimeRotation = Logme::TIME_ROTATION_DAILY;
  ApplyConfig(config);

  const std::string payload(HOT_PATH_RECORD_SIZE, 'D');
  const auto before = Logme::FileBackend::GetCounters();
  const auto started = std::chrono::steady_clock::now();

  LogRecords(payload, HOT_PATH_ITERATIONS);

  const auto elapsed = std::chrono::steady_clock::now() - started;
  const auto after = Logme::FileBackend::GetCounters();
  const CounterDelta delta = DiffCounters(before, after);

  PrintBenchmark("daily rotation same period", NsPerCall(elapsed, HOT_PATH_ITERATIONS), delta);

  EXPECT_EQ(delta.AppendCalls, HOT_PATH_ITERATIONS);
  EXPECT_TRUE(delta.InputBytes >= HOT_PATH_ITERATIONS * payload.size());
  EXPECT_EQ(delta.ChangePartCalls, 0u);
  EXPECT_EQ(delta.ChangePartFailures, 0u);
  EXPECT_EQ(delta.SizeLimitCompletionCalls, 0u);
  EXPECT_EQ(delta.TimeLimitCompletionCalls, 0u);
  EXPECT_EQ(delta.ArchivedFiles, 0u);
  EXPECT_EQ(delta.CompressionSubmitCalls, 0u);
  EXPECT_EQ(delta.RetentionRuns, 0u);
  EXPECT_EQ(delta.WriteErrors, 0u);
  EXPECT_FALSE(fs::exists(Archive1));
  EXPECT_FALSE(fs::exists(Archive2));
}

TEST_F(FileBackendHotPathTest, SizeLimitCompletionIsVisibleInCounters)
{
  auto config = MakeConfig(Logme::SIZE_LIMIT_ROTATE, 2048);
  ApplyConfig(config);

  const std::string first(1500, 'A');
  const std::string second(1500, 'B');
  const auto before = Logme::FileBackend::GetCounters();

  Backend->AppendString(first.data(), first.size());
  Backend->AppendString(second.data(), second.size());
  Backend->Flush();

  const auto after = Logme::FileBackend::GetCounters();
  const CounterDelta delta = DiffCounters(before, after);

  EXPECT_EQ(delta.AppendCalls, 2u);
  EXPECT_EQ(delta.ChangePartCalls, 1u);
  EXPECT_EQ(delta.SizeLimitCompletionCalls, 1u);
  EXPECT_EQ(delta.TimeLimitCompletionCalls, 0u);
  EXPECT_EQ(delta.ChangePartFailures, 0u);
  EXPECT_EQ(delta.ArchivedFiles, 1u);
  EXPECT_EQ(delta.CompressionSubmitCalls, 0u);
  EXPECT_EQ(delta.RetentionRuns, 0u);
  EXPECT_EQ(delta.WriteErrors, 0u);
  ASSERT_TRUE(fs::exists(Archive1));
  ASSERT_TRUE(fs::exists(Active));
}
