#pragma once

#include <memory>
#include <mutex>

#include <Logme/Debug/DebugManager.h>

namespace Logme
{
  class DebugManagerFactory
  {
    std::mutex Lock;
    std::shared_ptr<DebugManager> Instance;

  public:
    DebugManagerFactory();
    ~DebugManagerFactory();

    void Add(const DebugBackendPtr& backend);
    void Remove(DebugBackend* backend);
    bool Push(const char* text, size_t len);
    bool PushAndFlush(const char* text, size_t len);
    void Flush();
    void SetStopping();
    size_t GetMemoryUsage();
  };
}
