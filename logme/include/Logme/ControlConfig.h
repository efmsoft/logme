#pragma once

namespace Logme
{

struct ControlConfig
{
  bool Enable = false;

  // Discovery is enabled by default when control server is enabled.
  bool DiscoveryEnable = true;
};

}
