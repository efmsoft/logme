#ifdef _WIN32
#include <winsock2.h>
#endif

#include <Logme/Backend/FileBackend.h>
#include <Logme/Channel.h>
#include <Logme/Logger.h>
#include <Logme/Logme.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace
{
  constexpr int CONTROL_PORT = 7791;
  constexpr int WORKER_COUNT = 4;
  constexpr std::uint64_t LARGE_MESSAGE_PERIOD = 64;
  constexpr std::uint64_t STATUS_MESSAGE_PERIOD = 512;

  std::atomic<bool> Running{true};

  Logme::ChannelPtr CreateChannel(const char* name)
  {
    auto channel = Logme::Instance->CreateChannel(
      Logme::ID{name}
      , Logme::OutputFlags()
      , Logme::LEVEL_DEBUG
    );

    channel->RemoveBackends();

    Logme::OutputFlags flags;
    flags.Value = 0;
    channel->SetFlags(flags);
    channel->SetFilterLevel(Logme::LEVEL_DEBUG);
    return channel;
  }

  Logme::FileBackendPtr AddFileBackend(
    const Logme::ChannelPtr& channel
    , const char* filename
  )
  {
    auto backend = std::make_shared<Logme::FileBackend>(channel);
    backend->SetAppend(false);
    backend->SetAsync(true);

    if (!backend->CreateLog(filename))
      return {};

    channel->AddBackend(backend);
    return backend;
  }

  void NoisyWorker(
    int worker_id
    , const Logme::ChannelPtr& worker_channel
    , const Logme::ChannelPtr& payload_channel
    , const Logme::ChannelPtr& fanout_channel
  )
  {
    std::uint64_t item_id = 0;
    const std::string payload(4096, static_cast<char>('A' + worker_id));

    while (Running.load(std::memory_order_relaxed))
    {
      // Deliberately noisy: this call site should dominate the record count.
      LogmeI(
        worker_channel
        , "Processing item: worker=%d item=%llu"
        , worker_id
        , static_cast<unsigned long long>(item_id)
      );

      if ((item_id % LARGE_MESSAGE_PERIOD) == 0)
      {
        // Deliberately large: this call site should be prominent when sorting by bytes.
        LogmeI(
          payload_channel
          , "Received payload: worker=%d size=%zu data=%s"
          , worker_id
          , payload.size()
          , payload.c_str()
        );
      }

      if ((item_id % STATUS_MESSAGE_PERIOD) == 0)
      {
        // This source channel fans out to two different file backends.
        LogmeI(
          fanout_channel
          , "Worker status: worker=%d item=%llu"
          , worker_id
          , static_cast<unsigned long long>(item_id)
        );
      }

      ++item_id;

      if ((item_id % 8) == 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  void PrintInstructions()
  {
    std::cout
      << "Log statistics profiling example\n"
      << "\n"
      << "The process deliberately generates three different logging patterns:\n"
      << "  1. many short records from one call site;\n"
      << "  2. fewer large records from another call site;\n"
      << "  3. one source channel routed to two file backends.\n"
      << "\n"
      << "Control server: 127.0.0.1:" << CONTROL_PORT << "\n"
      << "\n"
      << "Before starting the workload, run these commands from another terminal:\n"
      << "  logmectl -p " << CONTROL_PORT << " logstat reset\n"
      << "  logmectl -p " << CONTROL_PORT << " logstat start\n"
      << "\n"
      << "Press ENTER to start the workload.\n"
      << std::endl;
  }
}

int main()
{
#ifdef _WIN32
  WSADATA wsa = {0};
  const int rc = WSAStartup(MAKEWORD(2, 2), &wsa);

  if (rc != 0)
  {
    std::cerr << "WSAStartup failed: " << rc << std::endl;
    return 1;
  }
#endif

  Logme::ControlConfig control{};
  control.Enable = true;
  control.Port = CONTROL_PORT;
  control.Interface = 0;

  if (!Logme::Instance->StartControlServer(control))
  {
    std::cerr << "Failed to start the Logme control server" << std::endl;
#ifdef _WIN32
    WSACleanup();
#endif
    return 2;
  }

  auto worker_channel = CreateChannel("profiling.worker");
  auto payload_channel = CreateChannel("profiling.payload");
  auto fanout_channel = CreateChannel("profiling.fanout");
  auto worker_file = AddFileBackend(worker_channel, "logstat-worker.log");
  auto payload_file = AddFileBackend(payload_channel, "logstat-payload.log");
  auto fanout_file_a = AddFileBackend(fanout_channel, "logstat-fanout-a.log");
  auto fanout_file_b = AddFileBackend(fanout_channel, "logstat-fanout-b.log");

  if (!worker_file || !payload_file || !fanout_file_a || !fanout_file_b)
  {
    std::cerr << "Failed to create one or more output files" << std::endl;
    Logme::Instance->StopControlServer();
#ifdef _WIN32
    WSACleanup();
#endif
    return 3;
  }


  PrintInstructions();
  std::cin.get();

  std::cout
    << "Workload is running. Let it run for approximately 15 to 30 seconds.\n"
    << "Then press ENTER here to stop the generators and flush all file backends.\n"
    << std::endl;

  std::vector<std::thread> workers;
  workers.reserve(WORKER_COUNT);

  for (int worker_id = 0; worker_id < WORKER_COUNT; ++worker_id)
  {
    workers.emplace_back(
      NoisyWorker
      , worker_id
      , worker_channel
      , payload_channel
      , fanout_channel
    );
  }

  std::cin.get();
  Running.store(false, std::memory_order_relaxed);

  for (auto& worker : workers)
    worker.join();

  worker_file->Flush();
  payload_file->Flush();
  fanout_file_a->Flush();
  fanout_file_b->Flush();

  std::cout
    << "Workload stopped and file backends flushed.\n"
    << "Now run these commands from the other terminal:\n"
    << "  logmectl -p " << CONTROL_PORT << " logstat status\n"
    << "  logmectl -p " << CONTROL_PORT << " logstat stop\n"
    << "  logmectl -p " << CONTROL_PORT << " logstat outputs --backend FileBackend --sort records --limit 10\n"
    << "  logmectl -p " << CONTROL_PORT << " logstat outputs --backend FileBackend --sort bytes --limit 10\n"
    << "  logmectl -p " << CONTROL_PORT << " logstat backends --sort bytes --limit 10\n"
    << "  logmectl -p " << CONTROL_PORT << " logstat files --sort written-bytes --limit 10\n"
    << "\n"
    << "Press ENTER to exit after reviewing the reports.\n"
    << std::endl;

  std::cin.get();

  worker_channel->RemoveBackends();
  payload_channel->RemoveBackends();
  fanout_channel->RemoveBackends();

  worker_file->CloseLog();
  payload_file->CloseLog();
  fanout_file_a->CloseLog();
  fanout_file_b->CloseLog();

  Logme::Instance->StopControlServer();

#ifdef _WIN32
  WSACleanup();
#endif

  return 0;
}
