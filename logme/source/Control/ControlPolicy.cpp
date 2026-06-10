#include <Logme/ControlPolicy.h>

using namespace Logme;

ControlPolicy::ControlPolicy()
  : Mode(ControlPolicyMode::FULL)
  , AllowExtensions(true)
  , AllowLogsCommand(true)
  , AllowFormatCommand(true)
  , AllowFileBackends(true)
  , AllowChannelCreateDelete(true)
  , AllowChannelRouting(true)
  , AllowChannelError(true)
  , AllowBackendChanges(true)
  , AllowLevelChanges(true)
  , AllowFlagChanges(true)
  , AllowTraceChanges(true)
  , AllowSubsystemChanges(true)
{
}

ControlPolicy ControlPolicy::Full()
{
  ControlPolicy policy;

  policy.Mode = ControlPolicyMode::FULL;
  policy.AllowExtensions = true;
  policy.AllowLogsCommand = true;
  policy.AllowFormatCommand = true;
  policy.AllowFileBackends = true;
  policy.AllowChannelCreateDelete = true;
  policy.AllowChannelRouting = true;
  policy.AllowChannelError = true;
  policy.AllowBackendChanges = true;
  policy.AllowLevelChanges = true;
  policy.AllowFlagChanges = true;
  policy.AllowTraceChanges = true;
  policy.AllowSubsystemChanges = true;

  return policy;
}

ControlPolicy ControlPolicy::Safe()
{
  ControlPolicy policy;

  policy.Mode = ControlPolicyMode::SAFE;
  policy.AllowExtensions = false;
  policy.AllowLogsCommand = false;
  policy.AllowFormatCommand = true;
  policy.AllowFileBackends = false;
  policy.AllowChannelCreateDelete = false;
  policy.AllowChannelRouting = false;
  policy.AllowChannelError = false;
  policy.AllowBackendChanges = false;
  policy.AllowLevelChanges = true;
  policy.AllowFlagChanges = true;
  policy.AllowTraceChanges = true;
  policy.AllowSubsystemChanges = true;

  return policy;
}

ControlPolicy ControlPolicy::Diagnostic()
{
  ControlPolicy policy = ControlPolicy::Safe();

  policy.Mode = ControlPolicyMode::DIAGNOSTIC;
  policy.AllowBackendChanges = true;

  return policy;
}
