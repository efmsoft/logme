#pragma once

#include <memory>
#include <mutex>

#include <Logme/Backend/ConsoleManager.h>

namespace Logme
{
  class ConsoleManagerFactory
  {
    std::mutex Lock;
    std::shared_ptr<ConsoleManager> Instance;

  public:
    ConsoleManagerFactory();
    ~ConsoleManagerFactory();

    void Add(
      const ConsoleBackendPtr& backend
      , size_t maxRecords
      , size_t maxBytes
      , ConsoleOverflowPolicy policy
    );
    void Remove(ConsoleBackend* backend);
    bool Push(
      ConsoleTarget target
      , Level level
      , bool highlight
      , const char* text
      , size_t len
    );
    bool AppendRedirected(
      const ChannelPtr& owner
      , ConsoleTarget target
      , const char* text
      , size_t len
    );
    void Flush();
    void Notify(ConsoleBackend* backend);
    void SetLimits(size_t maxRecords, size_t maxBytes);
    void SetOverflowPolicy(ConsoleOverflowPolicy policy);
    ConsoleQueueCounters GetCounters();
    void SetStopping();
  };
}
