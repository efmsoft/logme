#pragma once

#include <memory>
#include <mutex>

#include <Logme/Console/ConsoleManager.h>

namespace Logme
{
  class ConsoleManagerFactory
  {
    std::mutex Lock;
    std::shared_ptr<ConsoleManager> Instance;

  public:
    ConsoleManagerFactory();
    ~ConsoleManagerFactory();

    void Add(const ConsoleBackendPtr& backend);
    void Remove(ConsoleBackend* backend);
    bool Push(
      ConsoleTarget target
      , Level level
      , bool highlight
      , const char* text
      , size_t len
    );
    bool PushAndFlush(
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
    void NotifySettingsChanged();
    ConsoleQueueCounters GetCounters();
    void SetStopping();
  };
}
