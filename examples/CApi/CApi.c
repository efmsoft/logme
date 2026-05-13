#include <Logme/LogmeC.h>

int main(void)
{
  LogmeSetChannelLevel(NULL, LOGME_C_ENUM_VALUE(LEVEL_DEBUG));

  LogmeCreateChannel("c-channel", LOGME_C_ENUM_VALUE(LEVEL_DEBUG));
  LogmeRemoveChannelBackends("c-channel");
  LogmeAddConsoleBackend("c-channel", 0);
  LogmeAddFileBackend("c-channel", "c-api-example.log", 1, 1024 * 1024, 0, 3);

  LogmeD("C debug message: %d", 1);
  LogmeI("C info message: %s", "text");
  LogmeW_Ch("c-channel", "C warning in a named channel");
  LogmeE_Sid("CAPI", "C error with subsystem");
  LogmeC_ChSid("c-channel", "CAPI", "C critical message");

  LogmeI_If(1, "conditional C message");

  static LogmeCOverride onceOverride = LOGME_C_OVERRIDE_INIT;
  onceOverride.MaxRepetitions = 1;
  onceOverride.Remove.Value = 0;
  onceOverride.Remove.Method = 1;
  LogmeI_Ovr(&onceOverride, "this C override message is printed once");
  LogmeI_Ovr(&onceOverride, "this C override message is suppressed");

  LogmeFlushChannel("c-channel");
  LogmeShutdown();
  return 0;
}
