#include <Logme/Logme.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

static void Worker(Logme::ChannelPtr ch, int workerId, int messages, std::atomic<int>& counter)
{
  // Set default thread channel so channel-less logs in this thread go to 'ch'.
  LogmeThreadChannel(ch->GetID());

  for (int i = 0; i < messages; i++)
  {
    // ChannelPtr is slightly faster than ID in multithreaded systems.
    LogmeI(ch, "worker=%d msg=%d", workerId, i);

    // This call does not specify a channel, but will still go to 'ch' because of LogmeThreadChannel above.
    LogmeD("worker=%d (default thread channel)", workerId);

    counter.fetch_add(1, std::memory_order_relaxed);
  }
}

int main()
{
  const int threads = 4;
  const int messagesPerThread = 1000;

  std::atomic<int> counter{0};

  auto t0 = std::chrono::steady_clock::now();

  std::vector<std::thread> workers;
  workers.reserve(threads);

  for (int i = 0; i < threads; i++)
  {
    // Create a dedicated channel for each thread.
    // Using separate channels reduces contention because no name lookup is required.
    auto ch = Logme::Instance->CreateChannel();
    ch->AddLink(::CH);

    workers.emplace_back(Worker, ch, i, messagesPerThread, std::ref(counter));
  }

  for (auto& t : workers)
  {
    t.join();
  }

  auto t1 = std::chrono::steady_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

  LogmeI("threads=%d messages=%d time_ms=%lld",
    threads,
    counter.load(std::memory_order_relaxed),
    (long long)ms
  );

  return 0;
}
