#pragma once

#include <Logme/Backend/Backend.h>
#include <Logme/Backend/FileBackend.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

namespace Logme
{
  enum class ConsoleOverflowPolicy
  {
    BLOCK,
    DROP_NEW,
    DROP_OLDEST,
    SYNC_FALLBACK,
  };

  enum class ConsoleTarget
  {
    STDOUT,
    STDERR,
  };

  struct ConsoleRecord
  {
    ConsoleTarget Target;
    Level ErrorLevel;
    bool Highlight;
    bool HasAnsi;
    std::string Text;
  };

  struct ConsoleQueueCounters
  {
    uint64_t QueuedRecords = 0;
    uint64_t QueuedBytes = 0;
    uint64_t MaxQueuedRecords = 0;
    uint64_t MaxQueuedBytes = 0;
    uint64_t DroppedRecords = 0;
    uint64_t DroppedBytes = 0;
    uint64_t BlockedCalls = 0;
    uint64_t SyncFallbackCalls = 0;
  };

  class ConsoleRecordQueue
  {
    std::mutex Lock;
    std::condition_variable NotFull;
    std::vector<ConsoleRecord> Records;
    size_t QueuedBytes;
    size_t MaxRecords;
    size_t MaxBytes;
    ConsoleOverflowPolicy OverflowPolicy;
    uint64_t DroppedRecords;
    uint64_t DroppedBytes;
    uint64_t BlockedCalls;
    uint64_t SyncFallbackCalls;
    uint64_t MaxQueuedRecords;
    uint64_t MaxQueuedBytes;

  public:
    ConsoleRecordQueue();

    void SetLimits(size_t maxRecords, size_t maxBytes);
    void SetOverflowPolicy(ConsoleOverflowPolicy policy);

    bool Push(ConsoleRecord&& record);
    bool TakeAll(std::vector<ConsoleRecord>& records);
    bool Empty() const;
    ConsoleQueueCounters GetCounters() const;
    void NotifySpace();
  };

  struct ConsoleBackend : public Backend
  {
    constexpr static const char* TYPE_ID = "ConsoleBackend";

    enum
    {
      QUEUE_RECORD_LIMIT = 8192,
      QUEUE_BYTE_LIMIT = 4 * 1024 * 1024,
    };

  private:
    bool Async;
    std::atomic<bool> Registered;
    std::atomic<bool> ShutdownFlag;
    std::atomic<bool> ShutdownCalled;

    ConsoleRecordQueue Queue;
    std::condition_variable Done;
    std::mutex DoneLock;

    FileBackendPtr RedirectStdout;
    FileBackendPtr RedirectStderr;
    bool RedirectStdoutChecked;
    bool RedirectStderrChecked;

  public:
    LOGMELNK ConsoleBackend(ChannelPtr owner);
    LOGMELNK ~ConsoleBackend();

    LOGMELNK void SetAsync(bool async);
    LOGMELNK bool GetAsync() const;
    LOGMELNK void SetQueueLimits(size_t maxRecords, size_t maxBytes);
    LOGMELNK void SetOverflowPolicy(ConsoleOverflowPolicy policy);
    LOGMELNK ConsoleQueueCounters GetQueueCounters() const;

    LOGMELNK void Display(Context& context) override;
    LOGMELNK void Flush() override;
    LOGMELNK void Freeze() override;
    LOGMELNK bool IsIdle() const override;

    LOGMELNK const char* GetEscapeSequence(Level level);

    LOGMELNK FILE* GetOutputStream(Context& context);
    LOGMELNK static void WriteText(FILE* stream, const char* text, size_t len, const char* escape);
    LOGMELNK static void RemoveAnsi(const char* text, size_t len, std::string& out);

  private:
    class ConsoleManagerFactory& GetFactory() const;
    FileBackendPtr GetRedirectBackend(ConsoleTarget target);
    bool AppendRedirected(ConsoleTarget target, const char* text, size_t len, bool hasAnsi);
    void RegisterIfNeeded();
    bool WorkerFunc();
    void OnShutdown();

    friend class ConsoleManager;
  };

  typedef std::shared_ptr<ConsoleBackend> ConsoleBackendPtr;
}
