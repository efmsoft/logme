#include <gtest/gtest.h>

#include <Logme/Backend/Backend.h>
#include <Logme/Backend/BufferBackend.h>
#include <Logme/Backend/FileBackend.h>
#include <Logme/Backend/RingBufferBackend.h>
#include <Logme/Channel.h>
#include <Logme/Logme.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace Logme;

extern "C" void LogStatisticsCWriteSiteA(int value);
extern "C" void LogStatisticsCWriteSiteB(const char* value);

namespace
{
  const size_t BENCHMARK_ITERATIONS = 100000;
  std::atomic<uint64_t> ActiveBackendChecks(0);

  struct NullBackend : public Backend
  {
    explicit NullBackend(const ChannelPtr& owner)
      : Backend(owner, "NullBackend")
    {
    }

    void Display(Context& context) override
    {
      int nc = 0;
      (void)context.Apply(Owner, Owner->GetFlags(), nc);

      if (Owner->GetOwner()->GetActiveLogStatisticsFast() != nullptr)
        ActiveBackendChecks.fetch_add(1, std::memory_order_relaxed);
    }
  };

  ChannelPtr ChannelA;
  ChannelPtr ChannelB;
  ChannelPtr OutputChannel;

  bool Contains(
    const std::string& text
    , const std::string& value
  )
  {
    return text.find(value) != std::string::npos;
  }

  uint64_t ReadUnsignedAfter(
    const std::string& text
    , const std::string& marker
  )
  {
    size_t position = text.find(marker);
    if (position == std::string::npos)
      return 0;

    position += marker.size();
    size_t end = position;
    while (end < text.size() && text[end] >= '0' && text[end] <= '9')
      ++end;

    if (end == position)
      return 0;

    return std::stoull(text.substr(position, end - position));
  }

  ChannelPtr MakeChannel(const char* name)
  {
    ChannelPtr channel = Instance->CreateChannel(ID{name});
    channel->RemoveBackends();

    OutputFlags flags;
    flags.Value = 0;
    channel->SetFlags(flags);
    channel->SetFilterLevel(LEVEL_DEBUG);
    channel->AddBackend(std::make_shared<NullBackend>(channel));
    return channel;
  }

  ChannelPtr MakeOutputChannel(const char* name)
  {
    ChannelPtr channel = Instance->CreateChannel(ID{name});
    channel->RemoveBackends();

    OutputFlags flags;
    flags.Value = 0;
    channel->SetFlags(flags);
    channel->SetFilterLevel(LEVEL_DEBUG);
    channel->AddBackend(std::make_shared<BufferBackend>(channel));
    return channel;
  }

  void LogSiteA(
    const ChannelPtr& channel
    , int value
  )
  {
    LogmeI(channel, "site-a value=%d", value);
  }

  void LogSiteB(
    const ChannelPtr& channel
    , const char* value
  )
  {
    LogmeW(channel, "site-b value=%s", value);
  }

  void BenchmarkSite(
    const ChannelPtr& channel
    , size_t iterations
  )
  {
    for (size_t i = 0; i < iterations; ++i)
      LogmeI(channel, "benchmark value=%zu", i);
  }

  double NanosecondsPerCall(
    std::chrono::steady_clock::duration duration
    , size_t iterations
  )
  {
    auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(
      duration
    ).count();

    return static_cast<double>(nanoseconds)
      / static_cast<double>(iterations);
  }

  void ResetStatistics()
  {
    Instance->StopLogStatistics();
    Instance->ResetLogStatistics();
  }
}

TEST(LogStatistics, CollectsTopSitesAndFormats)
{
  ResetStatistics();
  Instance->StartLogStatistics();

  LogSiteA(ChannelA, 1);
  LogSiteA(ChannelA, 2);
  LogSiteA(ChannelA, 3);
  LogSiteB(ChannelA, "abcdefghijklmnopqrstuvwxyz");
  LogSiteB(ChannelA, "abcdefghijklmnopqrstuvwxyz");

  Instance->StopLogStatistics();

  std::string byRecords = Instance->DumpLogStatisticsTop(
    LogStatisticsSort::RECORDS
    , 10
  );

  EXPECT_TRUE(Contains(byRecords, "LogSiteA"));
  EXPECT_TRUE(Contains(byRecords, "site-a value=%d"));
  EXPECT_TRUE(Contains(byRecords, "records=3"));
  EXPECT_TRUE(Contains(byRecords, "channel=log_statistics_a"));

  std::string byBytes = Instance->DumpLogStatisticsTop(
    LogStatisticsSort::MESSAGE_BYTES
    , 10
  );

  EXPECT_TRUE(Contains(byBytes, "LogSiteB"));
  EXPECT_TRUE(Contains(byBytes, "site-b value=%s"));
  EXPECT_TRUE(Contains(byBytes, "level=WARN"));
}

TEST(LogStatistics, AggregatesChannels)
{
  ResetStatistics();
  Instance->StartLogStatistics();

  LogSiteA(ChannelA, 1);
  LogSiteA(ChannelA, 2);
  LogSiteA(ChannelB, 3);

  Instance->StopLogStatistics();

  std::string channels = Instance->DumpLogStatisticsChannels(
    LogStatisticsSort::RECORDS
    , 10
  );

  EXPECT_TRUE(Contains(channels, "channel=log_statistics_a"));
  EXPECT_TRUE(Contains(channels, "channel=log_statistics_b"));
  EXPECT_TRUE(Contains(channels, "records=2"));
  EXPECT_TRUE(Contains(channels, "records=1"));
}

TEST(LogStatistics, CollectsDestinationBackendOutput)
{
  ResetStatistics();
  Instance->StartLogStatistics();

  LogSiteA(OutputChannel, 11);
  LogSiteA(OutputChannel, 22);

  Instance->StopLogStatistics();

  std::string outputs = Instance->DumpLogStatisticsOutputs(
    LogStatisticsSort::MESSAGE_BYTES
    , 10
    , BufferBackend::TYPE_ID
  );

  EXPECT_TRUE(Contains(outputs, "LogSiteA"));
  EXPECT_TRUE(Contains(outputs, "backend=BufferBackend"));
  EXPECT_TRUE(Contains(outputs, "channel=log_statistics_output"));
  EXPECT_TRUE(Contains(outputs, "records=2"));
  EXPECT_TRUE(Contains(outputs, "output-bytes="));
}

TEST(LogStatistics, SeparatesNativeCCallSites)
{
  ResetStatistics();
  ChannelPtr channel = MakeOutputChannel("log_statistics_c");

  Instance->StartLogStatistics();
  LogStatisticsCWriteSiteA(1);
  LogStatisticsCWriteSiteA(2);
  LogStatisticsCWriteSiteB("value");
  Instance->StopLogStatistics();

  std::string top = Instance->DumpLogStatisticsTop(
    LogStatisticsSort::RECORDS
    , 10
  );

  EXPECT_TRUE(Contains(top, "LogStatisticsCWriteSiteA"));
  EXPECT_TRUE(Contains(top, "c-site-a value=%d"));
  EXPECT_TRUE(Contains(top, "records=2"));
  EXPECT_TRUE(Contains(top, "LogStatisticsCWriteSiteB"));
  EXPECT_TRUE(Contains(top, "c-site-b value=%s"));

  std::string outputs = Instance->DumpLogStatisticsOutputs(
    LogStatisticsSort::RECORDS
    , 10
    , BufferBackend::TYPE_ID
  );

  EXPECT_TRUE(Contains(outputs, "LogStatisticsCWriteSiteA"));
  EXPECT_TRUE(Contains(outputs, "LogStatisticsCWriteSiteB"));
  EXPECT_TRUE(Contains(outputs, "channel=log_statistics_c"));
}

TEST(LogStatistics, AttributesLinkedBackendFanOut)
{
  ResetStatistics();

  ChannelPtr destination = MakeOutputChannel(
    "log_statistics_link_destination"
  );
  destination->AddBackend(std::make_shared<RingBufferBackend>(destination));

  ChannelPtr source = MakeChannel("log_statistics_link_source");
  source->RemoveBackends();
  source->AddLink(destination);

  Instance->StartLogStatistics();
  LogSiteA(source, 1);
  LogSiteA(source, 2);
  Instance->StopLogStatistics();

  std::string outputs = Instance->DumpLogStatisticsOutputs(
    LogStatisticsSort::RECORDS
    , 10
  );

  EXPECT_TRUE(Contains(outputs, "channel=log_statistics_link_destination"));
  EXPECT_TRUE(Contains(outputs, "backend=BufferBackend"));
  EXPECT_TRUE(Contains(outputs, "backend=RingBufferBackend"));
  EXPECT_TRUE(Contains(outputs, "records=2"));
}

TEST(LogStatistics, AggregatesDestinationBackends)
{
  ResetStatistics();
  Instance->StartLogStatistics();

  LogSiteA(OutputChannel, 1);
  LogSiteB(OutputChannel, "backend-output");
  LogSiteB(OutputChannel, "backend-output");

  Instance->StopLogStatistics();

  std::string backends = Instance->DumpLogStatisticsBackends(
    LogStatisticsSort::RECORDS
    , 10
    , BufferBackend::TYPE_ID
  );

  EXPECT_TRUE(Contains(backends, "backend=BufferBackend"));
  EXPECT_TRUE(Contains(backends, "channel=log_statistics_output"));
  EXPECT_TRUE(Contains(backends, "records=3"));
}

TEST(LogStatistics, CollectsAsyncFileBackendWorkerStatistics)
{
  ResetStatistics();

  std::filesystem::path path = std::filesystem::temp_directory_path()
    / "logme-log-statistics-file-backend.log";
  std::error_code error;
  std::filesystem::remove(path, error);

  ChannelPtr channel = Instance->CreateChannel(ID{"log_statistics_file"});
  channel->RemoveBackends();

  OutputFlags flags;
  flags.Value = 0;
  channel->SetFlags(flags);
  channel->SetFilterLevel(LEVEL_DEBUG);

  auto backend = std::make_shared<FileBackend>(channel);
  backend->SetAsync(true);
  ASSERT_TRUE(backend->CreateLog(path.string().c_str()));
  channel->AddBackend(backend);

  Instance->StartLogStatistics();
  LogSiteA(channel, 101);
  LogSiteA(channel, 202);
  backend->Flush();
  Instance->StopLogStatistics();

  std::string files = Instance->DumpLogStatisticsFileBackends(
    LogFileStatisticsSort::WRITTEN_BYTES
    , 10
  );

  EXPECT_TRUE(Contains(files, "Async FileBackend statistics"));
  EXPECT_TRUE(Contains(files, "channel=log_statistics_file"));
  EXPECT_TRUE(Contains(files, "accepted: records=2"));
  EXPECT_TRUE(Contains(files, "worker: batches=1"));
  EXPECT_TRUE(Contains(files, "operations=1"));
  EXPECT_TRUE(Contains(files, "failures: batches=0 operations=0"));
  EXPECT_TRUE(Contains(files, "queue-dropped: records=0 bytes=0"));

  uint64_t writtenBytes = ReadUnsignedAfter(
    files
    , "   written: buffers=1 bytes="
  );
  EXPECT_EQ(writtenBytes, std::filesystem::file_size(path));

  EXPECT_TRUE(Contains(
    Instance->Control("logstat files --sort batches --limit 5")
    , "channel=log_statistics_file"
  ));

  channel->RemoveBackends();
  backend->CloseLog();
  std::filesystem::remove(path, error);
}

TEST(LogStatistics, ReportsAsyncFileFailuresAndQueueDrops)
{
  ResetStatistics();

  ChannelPtr channel = MakeChannel("log_statistics_file_errors");
  auto backend = std::make_shared<FileBackend>(channel);

  Instance->StartLogStatistics();
  Instance->RecordFileBackendQueueDrop(channel.get(), backend.get(), 128);
  Instance->RecordFileBackendWrite(
    channel.get()
    , backend.get()
    , 3
    , 4096
    , 2
    , 1
    , 1024
    , 1
    , 2
    , 3072
  );
  Instance->StopLogStatistics();

  std::string files = Instance->DumpLogStatisticsFileBackends(
    LogFileStatisticsSort::WRITE_ERRORS
    , 10
  );

  EXPECT_TRUE(Contains(files, "failures: batches=1 operations=1"));
  EXPECT_TRUE(Contains(files, "buffers=2 input-bytes=3072"));
  EXPECT_TRUE(Contains(files, "queue-dropped: records=1 bytes=128"));
}

TEST(LogStatistics, DoesNotCollectWhileDisabled)
{
  ResetStatistics();

  for (int i = 0; i < 100; ++i)
    LogSiteA(ChannelA, i);

  std::string status = Instance->DumpLogStatisticsStatus();
  EXPECT_TRUE(Contains(status, "Records: 0"));
  EXPECT_TRUE(Contains(status, "Message bytes: 0"));
}

TEST(LogStatistics, SkipsBackendStatisticsPathWhileDisabled)
{
  ResetStatistics();
  ActiveBackendChecks.store(0, std::memory_order_relaxed);

  LogSiteA(ChannelA, 1);
  EXPECT_EQ(ActiveBackendChecks.load(std::memory_order_relaxed), 0U);

  Instance->StartLogStatistics();
  LogSiteA(ChannelA, 2);
  Instance->StopLogStatistics();

  EXPECT_EQ(ActiveBackendChecks.load(std::memory_order_relaxed), 1U);
}

TEST(LogStatistics, ResetPreservesActiveState)
{
  ResetStatistics();
  Instance->StartLogStatistics();

  LogSiteA(ChannelA, 1);
  LogSiteA(ChannelA, 2);

  Instance->ResetLogStatistics();
  EXPECT_TRUE(Instance->IsLogStatisticsActive());

  LogSiteA(ChannelA, 3);
  Instance->StopLogStatistics();

  std::string top = Instance->DumpLogStatisticsTop(
    LogStatisticsSort::RECORDS
    , 10
  );

  EXPECT_TRUE(Contains(top, "records=1"));
  EXPECT_FALSE(Contains(top, "records=3"));
}

TEST(LogStatistics, SupportsConcurrentRecording)
{
  ResetStatistics();
  Instance->StartLogStatistics();

  const size_t threadCount = 4;
  const size_t recordsPerThread = 1000;
  std::vector<std::thread> threads;

  for (size_t thread = 0; thread < threadCount; ++thread)
  {
    threads.emplace_back(
      []()
      {
        for (size_t i = 0; i < recordsPerThread; ++i)
          LogSiteA(ChannelA, static_cast<int>(i));
      }
    );
  }

  for (std::thread& thread : threads)
    thread.join();

  Instance->StopLogStatistics();

  std::string top = Instance->DumpLogStatisticsTop(
    LogStatisticsSort::RECORDS
    , 10
  );

  EXPECT_TRUE(Contains(top, "records=4000"));
}

TEST(LogStatistics, ControlCommandProvidesTextAndJsonResponses)
{
  ResetStatistics();

  EXPECT_TRUE(Contains(Instance->Control("logstat start"), "started"));
  LogSiteA(ChannelA, 7);
  EXPECT_TRUE(Contains(Instance->Control("logstat top --sort records --limit 5"), "LogSiteA"));
  LogSiteA(OutputChannel, 8);
  EXPECT_TRUE(Contains(
    Instance->Control("logstat outputs --backend BufferBackend --sort bytes --limit 5")
    , "backend=BufferBackend"
  ));
  EXPECT_TRUE(Contains(
    Instance->Control("logstat backends --backend BufferBackend --sort records")
    , "channel=log_statistics_output"
  ));
  EXPECT_TRUE(Contains(Instance->Control("logstat stop"), "stopped"));

  std::string json = Instance->Control("format json logstat status");
  EXPECT_TRUE(Contains(json, "\"ok\":true"));
  EXPECT_TRUE(Contains(json, "Log statistics"));
}

TEST(LogStatistics, DisabledHotPathBenchmark)
{
  BenchmarkSite(ChannelA, 1000);

  ResetStatistics();
  auto disabledStarted = std::chrono::steady_clock::now();
  BenchmarkSite(ChannelA, BENCHMARK_ITERATIONS);
  auto disabledDuration = std::chrono::steady_clock::now() - disabledStarted;

  Instance->StartLogStatistics();
  auto enabledStarted = std::chrono::steady_clock::now();
  BenchmarkSite(ChannelA, BENCHMARK_ITERATIONS);
  auto enabledDuration = std::chrono::steady_clock::now() - enabledStarted;
  Instance->StopLogStatistics();

  double disabledNs = NanosecondsPerCall(
    disabledDuration
    , BENCHMARK_ITERATIONS
  );
  double enabledNs = NanosecondsPerCall(
    enabledDuration
    , BENCHMARK_ITERATIONS
  );

  std::cout
    << "[ LOGSTAT HOTPATH ] disabled="
    << disabledNs
    << " ns/call enabled="
    << enabledNs
    << " ns/call overhead="
    << (enabledNs - disabledNs)
    << " ns/call"
    << std::endl;

  EXPECT_GT(disabledNs, 0.0);
  EXPECT_GT(enabledNs, 0.0);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  ChannelA = MakeChannel("log_statistics_a");
  ChannelB = MakeChannel("log_statistics_b");
  OutputChannel = MakeOutputChannel("log_statistics_output");

  return RUN_ALL_TESTS();
}
