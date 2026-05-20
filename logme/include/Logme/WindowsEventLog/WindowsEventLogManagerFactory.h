#pragma once

#include <memory>
#include <mutex>

#include <Logme/WindowsEventLog/WindowsEventLogManager.h>

namespace Logme
{
  class WindowsEventLogManagerFactory
  {
    std::mutex Lock;
    std::shared_ptr<WindowsEventLogManager> Instance;

  public:
    WindowsEventLogManagerFactory();
    ~WindowsEventLogManagerFactory();

    void Add(const WindowsEventLogBackendPtr& backend);
    void Remove(WindowsEventLogBackend* backend);
    bool Push(WindowsEventLogBackend* backend, Level level, const char* text, size_t len);
    bool PushAndFlush(WindowsEventLogBackend* backend, Level level, const char* text, size_t len);
    void Flush();
    void SetStopping();
    size_t GetMemoryUsage();
  };
}
