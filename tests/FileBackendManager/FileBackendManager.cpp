#include <gtest/gtest.h>

#include <Logme/Backend/FileBackend.h>
#include <Logme/File/FileManager.h>
#include <Logme/Logme.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace
{
  std::atomic<unsigned> FileIndex(0);

  std::filesystem::path MakePath(const char* name)
  {
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    auto index = FileIndex.fetch_add(1, std::memory_order_relaxed);
    auto file = std::string("logme-file-manager-")
      + name
      + "-"
      + std::to_string(now)
      + "-"
      + std::to_string(index)
      + ".log";
    return std::filesystem::temp_directory_path() / file;
  }

  void RemoveIfExists(const std::filesystem::path& path)
  {
    std::error_code ec;
    std::filesystem::remove(path, ec);
  }

  std::string ReadFile(const std::filesystem::path& path)
  {
    std::ifstream input(path, std::ios::binary);
    return std::string(
      std::istreambuf_iterator<char>(input)
      , std::istreambuf_iterator<char>()
    );
  }

  bool WaitForText(
    const std::filesystem::path& path
    , const std::string& text
    , std::chrono::milliseconds timeout
  )
  {
    auto deadline = std::chrono::steady_clock::now() + timeout;

    while (std::chrono::steady_clock::now() < deadline)
    {
      if (ReadFile(path).find(text) != std::string::npos)
        return true;

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return ReadFile(path).find(text) != std::string::npos;
  }

  bool WaitForManagerIdle(std::chrono::milliseconds timeout)
  {
    auto deadline = std::chrono::steady_clock::now() + timeout;

    while (std::chrono::steady_clock::now() < deadline)
    {
      auto counters = Logme::FileManager::GetCounters();
      if (counters.CurrentActiveDepth == 0)
        return true;

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return Logme::FileManager::GetCounters().CurrentActiveDepth == 0;
  }

  class FlushAfterGuard
  {
    uint64_t OldValue;

  public:
    explicit FlushAfterGuard(uint64_t value)
      : OldValue(Logme::FileBackend::GetFlushAfterDefault())
    {
      Logme::FileBackend::SetFlushAfterDefault(value);
    }

    ~FlushAfterGuard()
    {
      Logme::FileBackend::SetFlushAfterDefault(OldValue);
    }
  };

  struct FileFixture
  {
    std::string ChannelName;
    Logme::ID ChannelId;
    Logme::ChannelPtr Channel;
    std::shared_ptr<Logme::FileBackend> Backend;
    std::filesystem::path Path;

    explicit FileFixture(const char* name)
      : ChannelName(name)
      , ChannelId{ChannelName.c_str()}
      , Path(MakePath(name))
    {
      RemoveIfExists(Path);

      Logme::OutputFlags flags;
      flags.Value = 0;
      flags.Eol = true;

      Channel = Logme::Instance->CreateChannel(ChannelId, flags, Logme::LEVEL_DEBUG);
      Channel->RemoveBackends();
      Channel->SetFlags(flags);
      Channel->SetFilterLevel(Logme::LEVEL_DEBUG);

      Backend = std::make_shared<Logme::FileBackend>(Channel);
      EXPECT_TRUE(Backend->CreateLog(Path.string().c_str()));
      Channel->AddBackend(Backend);
    }

    ~FileFixture()
    {
      if (Backend)
        Backend->Flush();

      if (Channel)
        Channel->RemoveBackends();

      RemoveIfExists(Path);
    }
  };
}

TEST(FileBackendManager, DelayedFlushUsesScheduledQueuePath)
{
  ASSERT_TRUE(WaitForManagerIdle(std::chrono::milliseconds(1000)));
  Logme::FileManager::ResetCounters();
  FlushAfterGuard flushAfter(40);

  {
    FileFixture file("file-manager-delayed");

    LogmeI(file.ChannelId, "delayed-message");

    ASSERT_TRUE(WaitForText(
      file.Path
      , "delayed-message"
      , std::chrono::milliseconds(1000)
    ));
    ASSERT_TRUE(WaitForManagerIdle(std::chrono::milliseconds(1000)));

    auto counters = Logme::FileManager::GetCounters();
    EXPECT_EQ(0u, counters.BackendScanItems);
    EXPECT_EQ(0u, counters.ActiveQueuePushFront);
    EXPECT_GE(counters.ActiveQueuePushBack, 1u);
    EXPECT_EQ(0u, counters.ActiveQueueSortedInsert);
    EXPECT_GE(counters.HeadScheduledSelected, 1u);
    EXPECT_GE(counters.DueScheduledRuns, 1u);
    EXPECT_EQ(0u, counters.CurrentActiveDepth);
    EXPECT_GE(counters.MaxActiveDepth, 1u);
  }
}

TEST(FileBackendManager, ManyDelayedBackendsUseTailFastPath)
{
  ASSERT_TRUE(WaitForManagerIdle(std::chrono::milliseconds(1000)));
  Logme::FileManager::ResetCounters();
  FlushAfterGuard flushAfter(80);

  constexpr int BACKENDS = 1000;
  std::vector<std::unique_ptr<FileFixture>> files;
  files.reserve(BACKENDS);

  for (int i = 0; i < BACKENDS; ++i)
  {
    auto name = std::string("file-manager-many-") + std::to_string(i);
    files.push_back(std::make_unique<FileFixture>(name.c_str()));
  }

  for (int i = 0; i < BACKENDS; ++i)
    LogmeI(files[i]->ChannelId, "many-backend-message-%d", i);

  ASSERT_TRUE(WaitForManagerIdle(std::chrono::milliseconds(5000)));

  auto counters = Logme::FileManager::GetCounters();
  EXPECT_EQ(0u, counters.BackendScanItems);
  EXPECT_EQ(0u, counters.ActiveQueueSortedInsert);
  EXPECT_GE(counters.ActiveQueuePushBack, (uint64_t)BACKENDS);
  EXPECT_GE(counters.HeadScheduledSelected, (uint64_t)BACKENDS);
  EXPECT_GE(counters.MaxActiveDepth, (uint64_t)BACKENDS / 2);
  EXPECT_EQ(0u, counters.CurrentActiveDepth);

  for (int i = 0; i < BACKENDS; ++i)
  {
    auto marker = std::string("many-backend-message-") + std::to_string(i);
    EXPECT_TRUE(WaitForText(
      files[i]->Path
      , marker
      , std::chrono::milliseconds(1000)
    )) << marker;
  }
}

TEST(FileBackendManager, ImmediateFlushMovesDelayedBackendToFront)
{
  ASSERT_TRUE(WaitForManagerIdle(std::chrono::milliseconds(1000)));
  Logme::FileManager::ResetCounters();
  FlushAfterGuard flushAfter(500);

  {
    FileFixture file("file-manager-move-front");

    LogmeI(file.ChannelId, "move-front-delayed-message");
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    file.Backend->Flush();

    ASSERT_TRUE(WaitForText(
      file.Path
      , "move-front-delayed-message"
      , std::chrono::milliseconds(1000)
    ));
    ASSERT_TRUE(WaitForManagerIdle(std::chrono::milliseconds(1000)));

    auto counters = Logme::FileManager::GetCounters();
    EXPECT_EQ(0u, counters.BackendScanItems);
    EXPECT_GE(counters.ActiveQueuePushBack, 1u);
    EXPECT_GE(counters.ActiveQueuePushFront, 1u);
    EXPECT_GE(counters.ActiveQueueReorder, 1u);
    EXPECT_GE(counters.HeadRightNowSelected, 1u);
    EXPECT_EQ(0u, counters.CurrentActiveDepth);
  }
}

TEST(FileBackendManager, ConcurrentBackendsDoNotLeakActiveQueueItems)
{
  ASSERT_TRUE(WaitForManagerIdle(std::chrono::milliseconds(1000)));
  Logme::FileManager::ResetCounters();
  FlushAfterGuard flushAfter(60);

  constexpr int BACKENDS = 100;
  constexpr int THREADS = 64;
  constexpr int MESSAGES = 24;
  std::vector<std::unique_ptr<FileFixture>> files;
  files.reserve(BACKENDS);

  for (int i = 0; i < BACKENDS; ++i)
  {
    auto name = std::string("file-manager-stress-") + std::to_string(i);
    files.push_back(std::make_unique<FileFixture>(name.c_str()));
  }

  std::vector<std::thread> threads;
  threads.reserve(THREADS);
  for (int i = 0; i < THREADS; ++i)
  {
    threads.emplace_back(
      [&, i]()
      {
        for (int j = 0; j < MESSAGES; ++j)
        {
          int backendIndex = (i + j) % BACKENDS;
          LogmeI(
            files[backendIndex]->ChannelId
            , "stress-thread-%d-message-%d"
            , i
            , j
          );

          if ((j % 7) == 0)
            files[backendIndex]->Backend->Flush();
        }
      }
    );
  }

  for (auto& thread : threads)
    thread.join();

  ASSERT_TRUE(WaitForManagerIdle(std::chrono::milliseconds(5000)));

  auto counters = Logme::FileManager::GetCounters();
  EXPECT_EQ(0u, counters.BackendScanItems);
  EXPECT_EQ(0u, counters.CurrentActiveDepth);
  EXPECT_GE(counters.ActiveQueuePushBack, 1u);
  EXPECT_GE(counters.ActiveQueuePushFront, 1u);
  EXPECT_GE(counters.ActiveQueueReorder, 1u);
  EXPECT_GE(counters.HeadScheduledSelected + counters.HeadRightNowSelected, 1u);

  EXPECT_TRUE(WaitForText(
    files.front()->Path
    , "stress-thread-0-message-0"
    , std::chrono::milliseconds(1000)
  ));
}
