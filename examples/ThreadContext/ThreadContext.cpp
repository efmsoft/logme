#include <Logme/Logme.h>

#include <thread>
#include <vector>

#if defined(_MSC_VER)
#pragma warning(disable: 4840)
#endif

static void LibraryWork(const char* tag)
{
  // This code does not know anything about channels.
  // If the caller sets LogmeThreadChannel, the messages will be routed there.
  LogmeI() << "LibraryWork: " << tag;
  LogmeW("LibraryWork warn: %s", tag);
}

static void Worker(int workerId)
{
  // Create a channel with an auto-generated name.
  // Using ChannelPtr is faster in multithreaded systems because it avoids name lookup.
  auto ch = Logme::Instance->CreateChannel();
  ch->AddLink(::CH);

  // Route all channel-less logs in this thread to 'ch'.
  LogmeThreadChannel(ch->GetID());

  Logme::Override ovr;
  ovr.Remove.Method = true;

  // Apply thread override for channel-less logs in this thread.
  LogmeThreadOverride(ovr);

  // Explicit channel (ChannelPtr) usage.
  LogmeI(ch) << "Worker " << workerId << ": explicit channel";
  LogmeI(ch, "Worker %d: printf style", workerId);

#ifndef LOGME_DISABLE_STD_FORMAT
  fLogmeI(ch, "Worker {}: std::format style", workerId);
#endif

  // Library logs without specifying channel/override.
  LibraryWork("start");
}

int main()
{
  const int threads = 4;

  std::vector<std::thread> workers;
  workers.reserve(threads);

  for (int i = 0; i < threads; i++)
  {
    workers.emplace_back(Worker, i);
  }

  for (auto& t : workers)
  {
    t.join();
  }

  return 0;
}
