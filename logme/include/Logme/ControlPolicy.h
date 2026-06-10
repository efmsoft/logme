#pragma once

#include <Logme/Types.h>

namespace Logme
{

  enum class ControlPolicyMode
  {
    FULL,
    SAFE,
    DIAGNOSTIC,
    CUSTOM
  };

  class ControlPolicy
  {
  public:
    ControlPolicyMode Mode;

    bool AllowExtensions;
    bool AllowLogsCommand;
    bool AllowFormatCommand;
    bool AllowFileBackends;
    bool AllowChannelCreateDelete;
    bool AllowChannelRouting;
    bool AllowChannelError;
    bool AllowBackendChanges;
    bool AllowLevelChanges;
    bool AllowFlagChanges;
    bool AllowTraceChanges;
    bool AllowSubsystemChanges;

    LOGMELNK ControlPolicy();

    LOGMELNK static ControlPolicy Full();
    LOGMELNK static ControlPolicy Safe();
    LOGMELNK static ControlPolicy Diagnostic();
  };

}
