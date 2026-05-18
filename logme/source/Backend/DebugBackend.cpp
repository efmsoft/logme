#include <Logme/Backend/DebugBackend.h>
#include <Logme/Backend/ConsoleBackend.h>
#include <Logme/Channel.h>

#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace Logme;

namespace
{
  class DebugManager
  {
  public:
    DebugManager()
      : Stopping(false)
      , MaxRecords(ConsoleBackend::QUEUE_RECORD_LIMIT)
      , MaxBytes(ConsoleBackend::QUEUE_BYTE_LIMIT)
      , QueuedBytes(0)
      , OverflowPolicy(ConsoleOverflowPolicy::BLOCK)
    {
      Worker = std::thread(&DebugManager::Run, this);
    }

    ~DebugManager()
    {
      {
        std::lock_guard guard(Lock);
        Stopping = true;
      }

      Wake.notify_all();

      if (Worker.joinable())
        Worker.join();
    }

    void Push(std::string&& text)
    {
      std::unique_lock locker(Lock);
      const size_t size = text.size();

      if (OverflowPolicy == ConsoleOverflowPolicy::BLOCK)
      {
        Space.wait(
          locker
          , [this, size]()
          {
            return Stopping || HasSpace(size);
          }
        );
      }
      else if (!HasSpace(size))
      {
        if (OverflowPolicy == ConsoleOverflowPolicy::DROP_NEW)
          return;

        while (!Records.empty() && !HasSpace(size))
        {
          QueuedBytes -= Records.front().size();
          Records.pop_front();
        }
      }

      if (Stopping || !HasSpace(size))
        return;

      QueuedBytes += size;
      Records.push_back(std::move(text));
      Wake.notify_one();
    }

    void Flush()
    {
      std::unique_lock locker(Lock);
      Empty.wait(
        locker
        , [this]()
        {
          return Records.empty() && QueuedBytes == 0;
        }
      );
    }

  private:
    bool HasSpace(size_t size) const
    {
      if (Records.size() >= MaxRecords)
        return false;

      if (QueuedBytes + size > MaxBytes)
        return false;

      return true;
    }

    void Run()
    {
      for (;;)
      {
        std::deque<std::string> local;
        {
          std::unique_lock locker(Lock);
          Wake.wait(
            locker
            , [this]()
            {
              return Stopping || !Records.empty();
            }
          );

          if (Stopping && Records.empty())
            break;

          local.swap(Records);
          QueuedBytes = 0;
          Space.notify_all();
        }

        for (const auto& item : local)
        {
#ifdef _WIN32
          OutputDebugStringA(item.c_str());
#else
          (void)item;
#endif
        }

        {
          std::lock_guard guard(Lock);
          if (Records.empty() && QueuedBytes == 0)
            Empty.notify_all();
        }
      }
    }

  private:
    std::mutex Lock;
    std::condition_variable Wake;
    std::condition_variable Space;
    std::condition_variable Empty;
    std::deque<std::string> Records;
    bool Stopping;
    size_t MaxRecords;
    size_t MaxBytes;
    size_t QueuedBytes;
    ConsoleOverflowPolicy OverflowPolicy;
    std::thread Worker;
  };

  DebugManager& GetDebugManager()
  {
    static DebugManager manager;
    return manager;
  }
}

DebugBackend::DebugBackend(ChannelPtr owner)
  : Backend(owner, TYPE_ID)
{
}

bool DebugBackend::IsAsyncSupported() const
{
  return true;
}

void DebugBackend::Flush()
{
  if (GetAsync())
    GetDebugManager().Flush();
}

std::string DebugBackend::FormatDetails()
{
  return GetAsync() ? "ASYNC" : "SYNC";
}

void DebugBackend::Display(Context& context)
{
  int nc;
  const char* buffer = context.Apply(Owner, Owner->GetFlags(), nc);

  if (GetAsync())
  {
    GetDebugManager().Push(std::string(buffer, (size_t)nc));
    return;
  }

#ifdef _WIN32
  OutputDebugStringA(buffer);
#endif
}
