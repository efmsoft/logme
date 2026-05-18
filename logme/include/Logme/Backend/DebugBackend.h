#pragma once

#include <atomic>
#include <memory>

#include <Logme/Backend/Backend.h>

namespace Logme
{
  struct DebugBackend : public Backend
  {
    constexpr static const char* TYPE_ID = "DebugBackend";

  private:
    std::atomic<bool> Registered;
    std::atomic<bool> ShutdownFlag;
    std::atomic<bool> ShutdownCalled;

  public:
    LOGMELNK DebugBackend(ChannelPtr owner);
    LOGMELNK ~DebugBackend();

    LOGMELNK bool IsAsyncSupported() const override;
    LOGMELNK void Flush() override;
    LOGMELNK void Freeze() override;
    LOGMELNK bool IsIdle() const override;
    LOGMELNK std::string FormatDetails() override;

    LOGMELNK void Display(Context& context) override;

  private:
    class DebugManagerFactory& GetFactory() const;
    void RegisterIfNeeded();
    void OnShutdown();
    void DoFreeze();

    friend class DebugManager;
  };

  typedef std::shared_ptr<DebugBackend> DebugBackendPtr;
}
