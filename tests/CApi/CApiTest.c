#include <Logme/LogmeC.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

static int FileContains(
  const char* fileName
  , const char* text
)
{
  FILE* f = fopen(fileName, "rb");
  if (f == NULL)
    return 0;

  char buffer[4096];
  size_t size = fread(buffer, 1, sizeof(buffer) - 1, f);
  fclose(f);

  buffer[size] = '\0';
  return strstr(buffer, text) != NULL;
}

int main(void)
{
  const char* fileName = "c-api-test.log";
  remove(fileName);

  if (!LogmeCreateChannel("c-test", LOGME_C_ENUM_VALUE(LEVEL_DEBUG)))
    return 1;

  LogmeRemoveChannelBackends("c-test");

  if (!LogmeAddConsoleBackend("c-test", 0))
    return 2;

  if (!LogmeAddFileBackend("c-test", fileName, 0, 1024ULL * 1024, 0, 3))
    return 3;

  LogmeSetChannelLevel(NULL, LOGME_C_ENUM_VALUE(LEVEL_DEBUG));
  LogmeSetChannelLevel("c-test", LOGME_C_ENUM_VALUE(LEVEL_DEBUG));

  LogmeD("debug from C: %d", 1);
  LogmeI("info from C");
  LogmeW_Ch("c-test", "warning from C channel");
  LogmeE_Sid("CAPI", "error from C subsystem");
  LogmeC_ChSid("c-test", "CAPI", "critical from C channel and subsystem");

  LogmeSetChannelEnabled("c-test", 0);
  LogmeI_If(1, "conditional C message");
  LogmeSetChannelEnabled("c-test", 1);

  LogmeCOverride onceOverride = LOGME_C_OVERRIDE_INIT;
  onceOverride.MaxRepetitions = 1;
  onceOverride.Remove.Value = 0;
  onceOverride.Remove.Method = 1;
  LogmeI_Ovr(&onceOverride, "C override message 1");
  LogmeI_Ovr(&onceOverride, "C override message 2");

  if (onceOverride.Repetitions != 1)
    return 4;

  LogmeCOverride rateOverride = LOGME_C_OVERRIDE_INIT;
  LogmeInitOverride(&rateOverride, -1, 1000);
  LogmeI_ChOvr("c-test", &rateOverride, "C rate-limited override message 1");
  LogmeI_ChOvr("c-test", &rateOverride, "C rate-limited override message 2");

  if (rateOverride.LastTime == 0)
    return 5;

  LogmeFlushChannel("c-test");
  LogmeShutdown();

  if (!FileContains(fileName, "warning from C channel"))
    return 6;

  if (!FileContains(fileName, "critical from C channel and subsystem"))
    return 7;

  remove(fileName);
  return 0;
}
