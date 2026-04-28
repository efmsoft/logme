#include <gtest/gtest.h>

#include <Logme/Backend/ConsoleBackend.h>
#include <Logme/Logme.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <future>
#include <iterator>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
  #include <io.h>
  #define LOGME_TEST_DUP _dup
  #define LOGME_TEST_DUP2 _dup2
  #define LOGME_TEST_FILENO _fileno
  #define LOGME_TEST_CLOSE _close
#else
  #include <unistd.h>
  #define LOGME_TEST_DUP dup
  #define LOGME_TEST_DUP2 dup2
  #define LOGME_TEST_FILENO fileno
  #define LOGME_TEST_CLOSE close
#endif

namespace
{
  const Logme::ID ConsoleTestChannel{"console-backend-test"};

  FILE* ReopenStream(
    FILE* stream
    , const std::filesystem::path& path
    , const char* mode
  )
  {
#ifdef _WIN32
    FILE* fp = nullptr;
    if (freopen_s(&fp, path.string().c_str(), mode, stream) != 0)
      return nullptr;

    return fp;
#else
    return freopen(path.string().c_str(), mode, stream);
#endif
  }

  class StreamRedirect
  {
    FILE* Stream = nullptr;
    int Fd = -1;
    int SavedFd = -1;

  public:
    StreamRedirect(FILE* stream, const std::filesystem::path& path)
      : Stream(stream)
      , Fd(LOGME_TEST_FILENO(stream))
      , SavedFd(LOGME_TEST_DUP(Fd))
    {
      EXPECT_GE(SavedFd, 0);
      FILE* fp = ReopenStream(stream, path, "wb");
      EXPECT_NE(fp, nullptr);
    }

    ~StreamRedirect()
    {
      if (Stream)
        fflush(Stream);

      if (SavedFd >= 0)
      {
        LOGME_TEST_DUP2(SavedFd, Fd);
        LOGME_TEST_CLOSE(SavedFd);
      }
    }

    StreamRedirect(const StreamRedirect&) = delete;
    StreamRedirect& operator=(const StreamRedirect&) = delete;
  };

  struct BackendFixture
  {
    Logme::ChannelPtr Channel;
    std::shared_ptr<Logme::ConsoleBackend> Backend;
  };

  std::string ReadFile(const std::filesystem::path& path)
  {
    std::ifstream input(path, std::ios::binary);
    return std::string(
      std::istreambuf_iterator<char>(input)
      , std::istreambuf_iterator<char>()
    );
  }

  std::filesystem::path MakePath(const char* name)
  {
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    auto file = std::string("logme-") + name + "-" + std::to_string(now) + ".txt";
    return std::filesystem::temp_directory_path() / file;
  }

  void RemoveIfExists(const std::filesystem::path& path)
  {
    std::error_code ec;
    std::filesystem::remove(path, ec);
  }

  BackendFixture CreateBackend(
    bool async
    , Logme::ConsoleStream stream = Logme::STREAM_WARNCERR
  )
  {
    Logme::OutputFlags flags;
    flags.Value = 0;
    flags.Eol = true;
    flags.Highlight = true;
    flags.Console = stream;

    auto channel = Logme::Instance->CreateChannel(ConsoleTestChannel, flags, Logme::LEVEL_DEBUG);
    channel->RemoveBackends();
    channel->SetFlags(flags);
    channel->SetFilterLevel(Logme::LEVEL_DEBUG);

    auto backend = std::make_shared<Logme::ConsoleBackend>(channel);
    backend->SetAsync(async);
    channel->AddBackend(backend);

    BackendFixture fixture;
    fixture.Channel = channel;
    fixture.Backend = backend;
    return fixture;
  }

  size_t CountText(const std::string& text, const std::string& what)
  {
    size_t count = 0;
    size_t pos = 0;

    for (;;)
    {
      pos = text.find(what, pos);
      if (pos == std::string::npos)
        break;

      ++count;
      pos += what.size();
    }

    return count;
  }

  Logme::ConsoleRecord MakeRecord(const char* text)
  {
    Logme::ConsoleRecord record;
    record.Target = Logme::ConsoleTarget::STDOUT;
    record.ErrorLevel = Logme::LEVEL_INFO;
    record.Highlight = false;
    record.HasAnsi = false;
    record.Text = text;
    return record;
  }
}

TEST(ConsoleBackendTest, RemoveAnsiStripsKnownSequences)
{
  std::string out;
  const char* text = "plain \x1b[31mred\x1b[0m done";
  Logme::ConsoleBackend::RemoveAnsi(text, std::strlen(text), out);
  EXPECT_EQ(out, "plain red done");
}

TEST(ConsoleBackendTest, RemoveAnsiKeepsPlainTextWithoutEscapes)
{
  std::string out;
  const char* text = "plain text only";
  Logme::ConsoleBackend::RemoveAnsi(text, std::strlen(text), out);
  EXPECT_EQ(out, text);
}

TEST(ConsoleBackendTest, SyncRedirectsStdoutAndStderr)
{
  auto stdoutPath = MakePath("sync-stdout");
  auto stderrPath = MakePath("sync-stderr");
  RemoveIfExists(stdoutPath);
  RemoveIfExists(stderrPath);

  {
    StreamRedirect stdoutRedirect(stdout, stdoutPath);
    StreamRedirect stderrRedirect(stderr, stderrPath);

    auto fixture = CreateBackend(false);

    LogmeI(ConsoleTestChannel, "sync-info");
    LogmeE(ConsoleTestChannel, "\x1b[31msync-error\x1b[0m");

    fixture.Backend->Flush();
    fflush(stdout);
    fflush(stderr);
  }

  auto out = ReadFile(stdoutPath);
  auto err = ReadFile(stderrPath);

  EXPECT_NE(out.find("sync-info"), std::string::npos);
  EXPECT_EQ(out.find("sync-error"), std::string::npos);
  EXPECT_NE(err.find("sync-error"), std::string::npos);
  EXPECT_EQ(err.find("\x1b[31m"), std::string::npos);

  RemoveIfExists(stdoutPath);
  RemoveIfExists(stderrPath);
}

TEST(ConsoleBackendTest, SyncStreamModeAllToStdout)
{
  auto stdoutPath = MakePath("sync-all-stdout");
  auto stderrPath = MakePath("sync-all-stderr");
  RemoveIfExists(stdoutPath);
  RemoveIfExists(stderrPath);

  {
    StreamRedirect stdoutRedirect(stdout, stdoutPath);
    StreamRedirect stderrRedirect(stderr, stderrPath);

    auto fixture = CreateBackend(false, Logme::STREAM_ALL2COUT);

    LogmeI(ConsoleTestChannel, "all-to-stdout-info");
    LogmeE(ConsoleTestChannel, "all-to-stdout-error");

    fixture.Backend->Flush();
    fflush(stdout);
    fflush(stderr);
  }

  auto out = ReadFile(stdoutPath);
  auto err = ReadFile(stderrPath);

  EXPECT_NE(out.find("all-to-stdout-info"), std::string::npos);
  EXPECT_NE(out.find("all-to-stdout-error"), std::string::npos);
  EXPECT_EQ(err.find("all-to-stdout-info"), std::string::npos);
  EXPECT_EQ(err.find("all-to-stdout-error"), std::string::npos);

  RemoveIfExists(stdoutPath);
  RemoveIfExists(stderrPath);
}

TEST(ConsoleBackendTest, AsyncRedirectsStdoutAndStderrThroughFileBackend)
{
  auto stdoutPath = MakePath("async-stdout");
  auto stderrPath = MakePath("async-stderr");
  RemoveIfExists(stdoutPath);
  RemoveIfExists(stderrPath);

  {
    StreamRedirect stdoutRedirect(stdout, stdoutPath);
    StreamRedirect stderrRedirect(stderr, stderrPath);

    auto fixture = CreateBackend(true);

    LogmeI(ConsoleTestChannel, "async-info");
    LogmeE(ConsoleTestChannel, "\x1b[31masync-error\x1b[0m");

    fixture.Backend->Flush();
    fflush(stdout);
    fflush(stderr);
  }

  auto out = ReadFile(stdoutPath);
  auto err = ReadFile(stderrPath);

  EXPECT_NE(out.find("async-info"), std::string::npos);
  EXPECT_EQ(out.find("async-error"), std::string::npos);
  EXPECT_NE(err.find("async-error"), std::string::npos);
  EXPECT_EQ(err.find("\x1b[31m"), std::string::npos);

  RemoveIfExists(stdoutPath);
  RemoveIfExists(stderrPath);
}

TEST(ConsoleBackendTest, AsyncFlushMakesRedirectedOutputVisible)
{
  auto stdoutPath = MakePath("async-flush");
  auto stderrPath = MakePath("async-flush-stderr");
  RemoveIfExists(stdoutPath);
  RemoveIfExists(stderrPath);

  {
    StreamRedirect stdoutRedirect(stdout, stdoutPath);
    StreamRedirect stderrRedirect(stderr, stderrPath);

    auto fixture = CreateBackend(true, Logme::STREAM_ALL2COUT);

    for (int i = 0; i < 32; ++i)
      LogmeI(ConsoleTestChannel, "[async-flush-message-%d]", i);

    fixture.Backend->Flush();
    fflush(stdout);
  }

  auto out = ReadFile(stdoutPath);

  for (int i = 0; i < 32; ++i)
  {
    auto marker = std::string("[async-flush-message-") + std::to_string(i) + "]";
    EXPECT_NE(out.find(marker), std::string::npos) << marker;
  }

  RemoveIfExists(stdoutPath);
  RemoveIfExists(stderrPath);
}

TEST(ConsoleBackendTest, AsyncRedirectHandlesConcurrentProducers)
{
  auto stdoutPath = MakePath("async-multithread");
  auto stderrPath = MakePath("async-multithread-stderr");
  RemoveIfExists(stdoutPath);
  RemoveIfExists(stderrPath);

  constexpr int THREADS = 4;
  constexpr int MESSAGES = 64;

  {
    StreamRedirect stdoutRedirect(stdout, stdoutPath);
    StreamRedirect stderrRedirect(stderr, stderrPath);

    auto fixture = CreateBackend(true, Logme::STREAM_ALL2COUT);
    std::vector<std::thread> threads;

    for (int t = 0; t < THREADS; ++t)
    {
      threads.emplace_back(
        [t]()
        {
          for (int i = 0; i < MESSAGES; ++i)
            LogmeI(ConsoleTestChannel, "[async-mt-%d-%d]", t, i);
        }
      );
    }

    for (auto& thread : threads)
      thread.join();

    fixture.Backend->Flush();
    fflush(stdout);
  }

  auto out = ReadFile(stdoutPath);

  for (int t = 0; t < THREADS; ++t)
  {
    for (int i = 0; i < MESSAGES; ++i)
    {
      auto marker = std::string("[async-mt-") + std::to_string(t) + "-" + std::to_string(i) + "]";
      EXPECT_EQ(CountText(out, marker), 1u) << marker;
    }
  }

  RemoveIfExists(stdoutPath);
  RemoveIfExists(stderrPath);
}

TEST(ConsoleBackendTest, QueueDropNewKeepsExistingRecords)
{
  Logme::ConsoleRecordQueue queue;
  queue.SetLimits(1, 0);
  queue.SetOverflowPolicy(Logme::ConsoleOverflowPolicy::DROP_NEW);

  EXPECT_TRUE(queue.Push(MakeRecord("first")));
  EXPECT_TRUE(queue.Push(MakeRecord("second")));

  std::vector<Logme::ConsoleRecord> records;
  EXPECT_TRUE(queue.TakeAll(records));
  ASSERT_EQ(records.size(), 1u);
  EXPECT_EQ(records[0].Text, "first");
}

TEST(ConsoleBackendTest, QueueDropOldestKeepsNewestRecords)
{
  Logme::ConsoleRecordQueue queue;
  queue.SetLimits(1, 0);
  queue.SetOverflowPolicy(Logme::ConsoleOverflowPolicy::DROP_OLDEST);

  EXPECT_TRUE(queue.Push(MakeRecord("first")));
  EXPECT_TRUE(queue.Push(MakeRecord("second")));

  std::vector<Logme::ConsoleRecord> records;
  EXPECT_TRUE(queue.TakeAll(records));
  ASSERT_EQ(records.size(), 1u);
  EXPECT_EQ(records[0].Text, "second");
}

TEST(ConsoleBackendTest, QueueSyncFallbackReportsFullQueue)
{
  Logme::ConsoleRecordQueue queue;
  queue.SetLimits(1, 0);
  queue.SetOverflowPolicy(Logme::ConsoleOverflowPolicy::SYNC_FALLBACK);

  EXPECT_TRUE(queue.Push(MakeRecord("first")));
  EXPECT_FALSE(queue.Push(MakeRecord("second")));

  std::vector<Logme::ConsoleRecord> records;
  EXPECT_TRUE(queue.TakeAll(records));
  ASSERT_EQ(records.size(), 1u);
  EXPECT_EQ(records[0].Text, "first");
}

TEST(ConsoleBackendTest, QueueBlockWaitsUntilSpaceIsAvailable)
{
  Logme::ConsoleRecordQueue queue;
  queue.SetLimits(1, 0);
  queue.SetOverflowPolicy(Logme::ConsoleOverflowPolicy::BLOCK);

  EXPECT_TRUE(queue.Push(MakeRecord("first")));

  auto pushed = std::async(
    std::launch::async
    , [&queue]()
    {
      return queue.Push(MakeRecord("second"));
    }
  );

  EXPECT_EQ(pushed.wait_for(std::chrono::milliseconds(50)), std::future_status::timeout);

  std::vector<Logme::ConsoleRecord> records;
  EXPECT_TRUE(queue.TakeAll(records));
  ASSERT_EQ(records.size(), 1u);
  EXPECT_EQ(records[0].Text, "first");

  EXPECT_TRUE(pushed.get());

  records.clear();
  EXPECT_TRUE(queue.TakeAll(records));
  ASSERT_EQ(records.size(), 1u);
  EXPECT_EQ(records[0].Text, "second");
}

TEST(ConsoleBackendTest, QueueByteLimitIsHonored)
{
  Logme::ConsoleRecordQueue queue;
  queue.SetLimits(0, 5);
  queue.SetOverflowPolicy(Logme::ConsoleOverflowPolicy::DROP_NEW);

  EXPECT_TRUE(queue.Push(MakeRecord("1234")));
  EXPECT_TRUE(queue.Push(MakeRecord("5678")));

  std::vector<Logme::ConsoleRecord> records;
  EXPECT_TRUE(queue.TakeAll(records));
  ASSERT_EQ(records.size(), 1u);
  EXPECT_EQ(records[0].Text, "1234");
}

TEST(ConsoleBackendTest, QueueCountersTrackDropNewOverflow)
{
  Logme::ConsoleRecordQueue queue;
  queue.SetLimits(1, 0);
  queue.SetOverflowPolicy(Logme::ConsoleOverflowPolicy::DROP_NEW);

  EXPECT_TRUE(queue.Push(MakeRecord("first")));
  EXPECT_TRUE(queue.Push(MakeRecord("second")));

  auto counters = queue.GetCounters();
  EXPECT_EQ(counters.QueuedRecords, 1u);
  EXPECT_EQ(counters.DroppedRecords, 1u);
  EXPECT_EQ(counters.DroppedBytes, 6u);
  EXPECT_EQ(counters.MaxQueuedRecords, 1u);
}

TEST(ConsoleBackendTest, QueueCountersTrackDropOldestOverflow)
{
  Logme::ConsoleRecordQueue queue;
  queue.SetLimits(1, 0);
  queue.SetOverflowPolicy(Logme::ConsoleOverflowPolicy::DROP_OLDEST);

  EXPECT_TRUE(queue.Push(MakeRecord("first")));
  EXPECT_TRUE(queue.Push(MakeRecord("second")));

  auto counters = queue.GetCounters();
  EXPECT_EQ(counters.QueuedRecords, 1u);
  EXPECT_EQ(counters.DroppedRecords, 1u);
  EXPECT_EQ(counters.DroppedBytes, 5u);
  EXPECT_EQ(counters.MaxQueuedRecords, 1u);

  std::vector<Logme::ConsoleRecord> records;
  EXPECT_TRUE(queue.TakeAll(records));
  ASSERT_EQ(records.size(), 1u);
  EXPECT_EQ(records[0].Text, "second");
}

TEST(ConsoleBackendTest, QueueCountersTrackSyncFallbackOverflow)
{
  Logme::ConsoleRecordQueue queue;
  queue.SetLimits(1, 0);
  queue.SetOverflowPolicy(Logme::ConsoleOverflowPolicy::SYNC_FALLBACK);

  EXPECT_TRUE(queue.Push(MakeRecord("first")));
  EXPECT_FALSE(queue.Push(MakeRecord("second")));

  auto counters = queue.GetCounters();
  EXPECT_EQ(counters.QueuedRecords, 1u);
  EXPECT_EQ(counters.SyncFallbackCalls, 1u);
  EXPECT_EQ(counters.DroppedRecords, 0u);
}

TEST(ConsoleBackendTest, QueueBlockDoesNotDropRecordsUnderOverflowPressure)
{
  Logme::ConsoleRecordQueue queue;
  queue.SetLimits(1, 0);
  queue.SetOverflowPolicy(Logme::ConsoleOverflowPolicy::BLOCK);

  constexpr int RECORDS = 16;

  EXPECT_TRUE(queue.Push(MakeRecord("record-0")));

  auto producer = std::async(
    std::launch::async
    , [&queue]()
    {
      for (int i = 1; i < RECORDS; ++i)
      {
        auto text = std::string("record-") + std::to_string(i);
        if (!queue.Push(MakeRecord(text.c_str())))
          return false;
      }

      return true;
    }
  );

  EXPECT_EQ(producer.wait_for(std::chrono::milliseconds(50)), std::future_status::timeout);

  std::vector<std::string> output;
  while (output.size() < static_cast<size_t>(RECORDS))
  {
    std::vector<Logme::ConsoleRecord> records;
    if (!queue.TakeAll(records))
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }

    for (auto& record : records)
      output.push_back(record.Text);
  }

  EXPECT_TRUE(producer.get());

  ASSERT_EQ(output.size(), static_cast<size_t>(RECORDS));
  for (int i = 0; i < RECORDS; ++i)
  {
    auto text = std::string("record-") + std::to_string(i);
    EXPECT_NE(std::find(output.begin(), output.end(), text), output.end()) << text;
  }

  auto counters = queue.GetCounters();
  EXPECT_GT(counters.BlockedCalls, 0u);
  EXPECT_EQ(counters.DroppedRecords, 0u);
  EXPECT_EQ(counters.SyncFallbackCalls, 0u);
}

TEST(ConsoleBackendTest, AsyncRedirectWithSmallQueueLimitKeepsAllMessages)
{
  auto stdoutPath = MakePath("async-small-limit");
  auto stderrPath = MakePath("async-small-limit-stderr");
  RemoveIfExists(stdoutPath);
  RemoveIfExists(stderrPath);

  constexpr int MESSAGES = 64;

  {
    StreamRedirect stdoutRedirect(stdout, stdoutPath);
    StreamRedirect stderrRedirect(stderr, stderrPath);

    auto fixture = CreateBackend(true, Logme::STREAM_ALL2COUT);
    fixture.Backend->SetQueueLimits(1, 1);
    fixture.Backend->SetOverflowPolicy(Logme::ConsoleOverflowPolicy::BLOCK);

    for (int i = 0; i < MESSAGES; ++i)
      LogmeI(ConsoleTestChannel, "[async-small-limit-%d]", i);

    fixture.Backend->Flush();
    fflush(stdout);
  }

  auto out = ReadFile(stdoutPath);

  for (int i = 0; i < MESSAGES; ++i)
  {
    auto marker = std::string("[async-small-limit-") + std::to_string(i) + "]";
    EXPECT_EQ(CountText(out, marker), 1u) << marker;
  }

  RemoveIfExists(stdoutPath);
  RemoveIfExists(stderrPath);
}
