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

    void Add(const ConsoleBackendPtr& backend);
    void Notify(ConsoleBackend* backend);
    void SetStopping();
  };
}
