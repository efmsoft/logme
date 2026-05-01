#pragma once

#include <Logme/Backend/Backend.h>
#include <Logme/Backend/FileBackend.h>

#include <atomic>
#include <condition_variable>
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

  enum ConsoleRecordFlags
  {
    CONSOLE_RECORD_HIGHLIGHT = 0x01,
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

    size_t QueueRecordLimit;
    size_t QueueByteLimit;
    ConsoleOverflowPolicy OverflowPolicy;

  public:
    LOGMELNK ConsoleBackend(ChannelPtr owner);
    LOGMELNK ~ConsoleBackend();

    LOGMELNK void SetAsync(bool async);
    LOGMELNK bool GetAsync() const;
    LOGMELNK void SetQueueLimits(size_t maxRecords, size_t maxBytes);
    LOGMELNK void SetOverflowPolicy(ConsoleOverflowPolicy policy);

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
    void RegisterIfNeeded();
    void OnShutdown();

    friend class ConsoleManager;
  };

  typedef std::shared_ptr<ConsoleBackend> ConsoleBackendPtr;
}
