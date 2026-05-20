#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

#include <Logme/Backend/Backend.h>

namespace Logme
{
  struct WindowsEventLogBackendConfig : public BackendConfig
  {
    std::string Source;
    uint32_t EventId;
    uint16_t Category;

    LOGMELNK WindowsEventLogBackendConfig();
    LOGMELNK bool Parse(const Json::Value* po) override;
  };

  struct WindowsEventLogBackend : public Backend
  {
    constexpr static const char* TYPE_ID = "WindowsEventLogBackend";

  private:
    std::string Source;
    uint32_t EventId;
    uint16_t Category;
    void* EventHandle;
    std::atomic<bool> Registered;
    std::atomic<bool> ShutdownFlag;
    std::atomic<bool> ShutdownCalled;

  public:
    LOGMELNK WindowsEventLogBackend(ChannelPtr owner);
    LOGMELNK ~WindowsEventLogBackend();

    LOGMELNK bool IsAsyncSupported() const override;
    LOGMELNK BackendConfigPtr CreateConfig() override;
    LOGMELNK bool ApplyConfig(BackendConfigPtr c) override;
    LOGMELNK std::string FormatDetails() override;

    LOGMELNK void Display(Context& context) override;
    LOGMELNK void Flush() override;
    LOGMELNK void Freeze() override;
    LOGMELNK bool IsIdle() const override;

    LOGMELNK const std::string& GetSource() const;
    LOGMELNK uint32_t GetEventId() const;
    LOGMELNK uint16_t GetCategory() const;

  private:
    class WindowsEventLogManagerFactory& GetFactory() const;
    void RegisterIfNeeded();
    void OnShutdown();
    void Close();
    bool WritePreparedRecord(Level level, uint32_t eventId, uint16_t category, const char* text, size_t len);

    friend class WindowsEventLogManager;
  };

  typedef std::shared_ptr<WindowsEventLogBackend> WindowsEventLogBackendPtr;
}
