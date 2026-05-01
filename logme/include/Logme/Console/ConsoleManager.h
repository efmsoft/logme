#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <Logme/Backend/ConsoleBackend.h>

#ifndef CONSOLE_ENABLE_COUNTERS
#define CONSOLE_ENABLE_COUNTERS 0
#endif

#if CONSOLE_ENABLE_COUNTERS
#define CONSOLE_CNT(x) x
#else
#define CONSOLE_CNT(x) ((void)0)
#endif

namespace Logme
{
  struct ConsoleRecordHeader
  {
    uint32_t Size;
    uint32_t TextSize;
    uint8_t Target;
    uint8_t Flags;
    uint16_t Reserved;
    int ErrorLevel;
  };

  struct ConsoleQueueCounters
  {
    uint64_t QueuedRecords;
    uint64_t QueuedBytes;
    uint64_t MaxQueuedRecords;
    uint64_t MaxQueuedBytes;
    uint64_t DroppedRecords;
    uint64_t DroppedBytes;
    uint64_t BlockedCalls;
    uint64_t SyncFallbackCalls;
    uint64_t RedirectedRecords;
    uint64_t RedirectedBytes;
  };

  class ConsoleManager
  {
    std::atomic<bool> StopRequested;
    bool Reschedule;
    std::thread ManagerThread;

    mutable std::mutex Lock;
    std::condition_variable CV;
    std::condition_variable NotFull;
    std::set<ConsoleBackend*> Backends;
    std::vector<unsigned char> Buffer;
    bool Processing;
    size_t QueuedRecords;
    size_t QueuedBytes;
    size_t MaxRecords;
    size_t MaxBytes;
    ConsoleOverflowPolicy OverflowPolicy;
#if CONSOLE_ENABLE_COUNTERS
    ConsoleQueueCounters Counters;
#endif

    FileBackendPtr RedirectStdout;
    FileBackendPtr RedirectStderr;
    bool RedirectStdoutChecked;
    bool RedirectStderrChecked;
    std::string RedirectStdoutName;
    std::string RedirectStderrName;

#ifdef _WIN32
    uint64_t ThreadID;
#endif

  public:
    ConsoleManager();
    ~ConsoleManager();

    void AddBackend(
      const ConsoleBackendPtr& backend
      , size_t maxRecords
      , size_t maxBytes
      , ConsoleOverflowPolicy policy
    );

    bool RemoveBackend(ConsoleBackend* backend);
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
    ConsoleQueueCounters GetCounters() const;

    bool Empty() const;
    bool Stopping() const;
    void SetStopping();

  private:
    bool HasSpace(size_t recordSize) const;
    bool DropOldest(size_t recordSize);
    FileBackendPtr GetRedirectBackend(
      const ChannelPtr& owner
      , ConsoleTarget target
    );
    void ManagementThread();
    void ProcessBuffer(std::vector<unsigned char>& buffer);
  };
}
