#include <gtest/gtest.h>

#include <Logme/Backend/FileBackend.h>
#include <Logme/File/FileManager.h>
#include <Logme/Logme.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
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
    auto file = std::string("logme-file-manager-counters-")
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

  template <typename Getter>
  bool WaitForCounterIncrease(
    Getter getter
    , uint64_t before
    , uint64_t delta
    , std::chrono::milliseconds timeout
  )
  {
    auto deadline = std::chrono::steady_clock::now() + timeout;

    while (std::chrono::steady_clock::now() < deadline)
    {
      if (getter() >= before + delta)
        return true;

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return getter() >= before + delta;
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

  class DataBufferCacheLimitGuard
  {
    size_t OldValue;

  public:
    explicit DataBufferCacheLimitGuard(size_t value)
      : OldValue(Logme::FileBackend::GetDataBufferCacheLimit())
    {
      if (value != 0)
        Logme::FileBackend::SetDataBufferCacheLimit(0);

      Logme::FileBackend::SetDataBufferCacheLimit(value);
    }

    ~DataBufferCacheLimitGuard()
    {
      Logme::FileBackend::SetDataBufferCacheLimit(OldValue);
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

      Channel = Logme::Instance->CreateChannel(
        ChannelId
        , flags
        , Logme::LEVEL_DEBUG
      );
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

  struct CounterDelta
  {
    Logme::FileManagerCounters Before;
    Logme::FileManagerCounters After;
  };

  uint64_t Diff(uint64_t before, uint64_t after)
  {
    return after - before;
  }

  void PrintCounters(const char* name, const CounterDelta& delta)
  {
    const auto& before = delta.Before;
    const auto& after = delta.After;

    std::cout
      << "[FileManagerCounters] " << name
      << " HeadRightNowChecks=" << Diff(before.HeadRightNowChecks, after.HeadRightNowChecks)
      << " HeadScheduledChecks=" << Diff(before.HeadScheduledChecks, after.HeadScheduledChecks)
      << " ActiveQueueStaleItems=" << Diff(before.ActiveQueueStaleItems, after.ActiveQueueStaleItems)
      << " ActiveQueuePushFront=" << Diff(before.ActiveQueuePushFront, after.ActiveQueuePushFront)
      << " ActiveQueuePushBack=" << Diff(before.ActiveQueuePushBack, after.ActiveQueuePushBack)
      << " ActiveQueueSortedInsert=" << Diff(before.ActiveQueueSortedInsert, after.ActiveQueueSortedInsert)
      << " ActiveQueueRemove=" << Diff(before.ActiveQueueRemove, after.ActiveQueueRemove)
      << " ActiveQueueReorder=" << Diff(before.ActiveQueueReorder, after.ActiveQueueReorder)
      << " HeadRightNowSelected=" << Diff(before.HeadRightNowSelected, after.HeadRightNowSelected)
      << " HeadScheduledSelected=" << Diff(before.HeadScheduledSelected, after.HeadScheduledSelected)
      << " DueScheduledRuns=" << Diff(before.DueScheduledRuns, after.DueScheduledRuns)
      << " ScheduledEarlyRuns=" << Diff(before.ScheduledEarlyRuns, after.ScheduledEarlyRuns)
      << " ScheduledLateRuns=" << Diff(before.ScheduledLateRuns, after.ScheduledLateRuns)
      << " MaxScheduledDelayMs=" << after.MaxScheduledDelayMs
      << " CurrentActiveDepth=" << after.CurrentActiveDepth
      << " MaxActiveDepth=" << after.MaxActiveDepth
      << " DataBufferAllocations=" << Diff(before.DataBufferAllocations, after.DataBufferAllocations)
      << " DataBufferReuses=" << Diff(before.DataBufferReuses, after.DataBufferReuses)
      << " DataBufferCacheHits=" << Diff(before.DataBufferCacheHits, after.DataBufferCacheHits)
      << " DataBufferCacheReturns=" << Diff(before.DataBufferCacheReturns, after.DataBufferCacheReturns)
      << std::endl;
  }
}

TEST(FileManagerCounters, MassiveDelayedBackendsUseTailFastPath)
{
  ASSERT_TRUE(WaitForManagerIdle(std::chrono::milliseconds(1000)));
  FlushAfterGuard flushAfter(80);

  constexpr int BACKENDS = 1000;
  auto before = Logme::FileManager::GetCounters();

  std::vector<std::unique_ptr<FileFixture>> files;
  files.reserve(BACKENDS);

  for (int i = 0; i < BACKENDS; ++i)
  {
    auto name = std::string("massive-delayed-") + std::to_string(i);
    files.push_back(std::make_unique<FileFixture>(name.c_str()));
  }

  for (int i = 0; i < BACKENDS; ++i)
  {
    LogmeI(
      files[i]->ChannelId
      , "massive-delayed-message-%d"
      , i
    );
  }

  ASSERT_TRUE(WaitForManagerIdle(std::chrono::milliseconds(10000)));

  auto after = Logme::FileManager::GetCounters();
  CounterDelta delta{before, after};
  PrintCounters("MassiveDelayedBackendsUseTailFastPath", delta);

  EXPECT_EQ(0u, Diff(before.ActiveQueueSortedInsert, after.ActiveQueueSortedInsert));
  EXPECT_GE(Diff(before.ActiveQueuePushBack, after.ActiveQueuePushBack), (uint64_t)BACKENDS);
  EXPECT_GE(Diff(before.HeadScheduledSelected, after.HeadScheduledSelected), (uint64_t)BACKENDS);
  EXPECT_GE(after.MaxActiveDepth, (uint64_t)BACKENDS / 2);
  EXPECT_EQ(0u, after.CurrentActiveDepth);

  EXPECT_TRUE(WaitForText(
    files.front()->Path
    , "massive-delayed-message-0"
    , std::chrono::milliseconds(1000)
  ));
}

TEST(FileManagerCounters, DelayedBackendMovesToFrontForImmediateFlush)
{
  ASSERT_TRUE(WaitForManagerIdle(std::chrono::milliseconds(1000)));
  FlushAfterGuard flushAfter(500);

  auto before = Logme::FileManager::GetCounters();

  {
    FileFixture file("delayed-to-immediate");

    LogmeI(file.ChannelId, "delayed-to-immediate-message");
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    file.Backend->Flush();

    ASSERT_TRUE(WaitForText(
      file.Path
      , "delayed-to-immediate-message"
      , std::chrono::milliseconds(1000)
    ));
    ASSERT_TRUE(WaitForManagerIdle(std::chrono::milliseconds(1000)));
  }

  auto after = Logme::FileManager::GetCounters();
  CounterDelta delta{before, after};
  PrintCounters("DelayedBackendMovesToFrontForImmediateFlush", delta);

  EXPECT_GE(Diff(before.ActiveQueuePushBack, after.ActiveQueuePushBack), 1u);
  EXPECT_GE(Diff(before.ActiveQueuePushFront, after.ActiveQueuePushFront), 1u);
  EXPECT_GE(Diff(before.HeadRightNowSelected, after.HeadRightNowSelected), 1u);
  EXPECT_EQ(0u, after.CurrentActiveDepth);
}

TEST(FileManagerCounters, ScheduledHeadWithinGapRunsWithoutSleep)
{
  ASSERT_TRUE(WaitForManagerIdle(std::chrono::milliseconds(1000)));
  FlushAfterGuard flushAfter(200);

  auto before = Logme::FileManager::GetCounters();

  {
    FileFixture first("scheduled-gap-first");
    FileFixture second("scheduled-gap-second");

    LogmeI(first.ChannelId, "scheduled-gap-first-message");
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
    LogmeI(second.ChannelId, "scheduled-gap-second-message");

    ASSERT_TRUE(WaitForManagerIdle(std::chrono::milliseconds(2000)));

    ASSERT_TRUE(WaitForText(
      first.Path
      , "scheduled-gap-first-message"
      , std::chrono::milliseconds(1000)
    ));
    ASSERT_TRUE(WaitForText(
      second.Path
      , "scheduled-gap-second-message"
      , std::chrono::milliseconds(1000)
    ));
  }

  ASSERT_TRUE(WaitForManagerIdle(std::chrono::milliseconds(1000)));

  auto after = Logme::FileManager::GetCounters();
  CounterDelta delta{before, after};
  PrintCounters("ScheduledHeadWithinGapRunsWithoutSleep", delta);

  uint64_t selected = Diff(
    before.HeadScheduledSelected
    , after.HeadScheduledSelected
  );
  uint64_t early = Diff(
    before.ScheduledEarlyRuns
    , after.ScheduledEarlyRuns
  );
  uint64_t late = Diff(
    before.ScheduledLateRuns
    , after.ScheduledLateRuns
  );

  EXPECT_GE(Diff(before.ActiveQueuePushBack, after.ActiveQueuePushBack), 2u);
  EXPECT_GE(selected, 2u);
  EXPECT_LE(selected, 4u);
  EXPECT_EQ(selected, early + late);
  EXPECT_LE(early, 2u);
  EXPECT_EQ(0u, after.CurrentActiveDepth);
}

TEST(FileManagerCounters, ConcurrentBackendsDoNotLeakActiveQueueItems)
{
  ASSERT_TRUE(WaitForManagerIdle(std::chrono::milliseconds(1000)));
  FlushAfterGuard flushAfter(60);

  constexpr int BACKENDS = 100;
  constexpr int THREADS = 64;
  constexpr int MESSAGES = 24;
  auto before = Logme::FileManager::GetCounters();

  std::vector<std::unique_ptr<FileFixture>> files;
  files.reserve(BACKENDS);

  for (int i = 0; i < BACKENDS; ++i)
  {
    auto name = std::string("concurrent-") + std::to_string(i);
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
            , "concurrent-thread-%d-message-%d"
            , i
            , j
          );

        }
      }
    );
  }

  for (auto& thread : threads)
    thread.join();

  ASSERT_TRUE(WaitForManagerIdle(std::chrono::milliseconds(10000)));

  auto after = Logme::FileManager::GetCounters();
  CounterDelta delta{before, after};
  PrintCounters("ConcurrentBackendsDoNotLeakActiveQueueItems", delta);

  EXPECT_EQ(0u, after.CurrentActiveDepth);
  EXPECT_GE(Diff(before.ActiveQueuePushBack, after.ActiveQueuePushBack), 1u);
  EXPECT_GE(Diff(before.ActiveQueuePushFront, after.ActiveQueuePushFront), 1u);
  EXPECT_GE(
    Diff(before.HeadScheduledSelected, after.HeadScheduledSelected)
      + Diff(before.HeadRightNowSelected, after.HeadRightNowSelected)
    , 1u
  );

  EXPECT_TRUE(WaitForText(
    files.front()->Path
    , "concurrent-thread-0-message-0"
    , std::chrono::milliseconds(1000)
  ));
}

TEST(FileManagerCounters, DataBufferCacheReusesInitialBuffer)
{
  ASSERT_TRUE(WaitForManagerIdle(std::chrono::milliseconds(1000)));

  DataBufferCacheLimitGuard cacheLimit(2);

  auto before = Logme::FileManager::GetCounters();

  {
    auto file = std::make_unique<FileFixture>("data-buffer-cache-first");
    LogmeI(file->ChannelId, "data-buffer-cache-first-message");
    ASSERT_TRUE(WaitForText(
      file->Path
      , "data-buffer-cache-first-message"
      , std::chrono::milliseconds(1000)
    ));
    file.reset();
  }

  ASSERT_TRUE(WaitForCounterIncrease(
    []
    {
      return Logme::FileManager::GetCounters().DataBufferCacheReturns;
    }
    , before.DataBufferCacheReturns
    , 1
    , std::chrono::milliseconds(2000)
  ));

  auto afterFirst = Logme::FileManager::GetCounters();
  EXPECT_GE(Diff(before.DataBufferAllocations, afterFirst.DataBufferAllocations), 1u);
  EXPECT_EQ(0u, Diff(before.DataBufferReuses, afterFirst.DataBufferReuses));
  EXPECT_LE(afterFirst.DataBufferCacheDepth, 2u);

  {
    auto file = std::make_unique<FileFixture>("data-buffer-cache-second");

    ASSERT_TRUE(WaitForCounterIncrease(
      []
      {
        return Logme::FileManager::GetCounters().DataBufferReuses;
      }
      , afterFirst.DataBufferReuses
      , 1
      , std::chrono::milliseconds(2000)
    ));

    LogmeI(file->ChannelId, "data-buffer-cache-second-message");
    ASSERT_TRUE(WaitForText(
      file->Path
      , "data-buffer-cache-second-message"
      , std::chrono::milliseconds(1000)
    ));
    file.reset();
  }

  ASSERT_TRUE(WaitForCounterIncrease(
    []
    {
      return Logme::FileManager::GetCounters().DataBufferCacheReturns;
    }
    , afterFirst.DataBufferCacheReturns
    , 1
    , std::chrono::milliseconds(2000)
  ));

  auto afterSecond = Logme::FileManager::GetCounters();
  EXPECT_GE(Diff(afterFirst.DataBufferReuses, afterSecond.DataBufferReuses), 1u);
  EXPECT_EQ(
    Diff(afterFirst.DataBufferCacheHits, afterSecond.DataBufferCacheHits)
    , Diff(afterFirst.DataBufferReuses, afterSecond.DataBufferReuses)
  );
  EXPECT_LE(afterSecond.DataBufferCacheDepth, 2u);
}
