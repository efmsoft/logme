#pragma once

#include <atomic>
#include <string>
#include <thread>

#include <Logme/Types.h>

namespace Logme
{
  class ControlDiscovery
  {
    ControlConfig Config;
    std::string EndpointName;
    std::thread Worker;
    std::atomic<bool> Stopped;

  public:
    explicit ControlDiscovery(const ControlConfig& config);
    ~ControlDiscovery();

    bool Start();
    void Stop();

  private:
    void WorkerFunc();
    std::string BuildResponse() const;
  };
}
