#include <Logme/LogmeC.h>

#include <Logme/Backend/ConsoleBackend.h>
#include <Logme/Backend/DebugBackend.h>
#include <Logme/Backend/FileBackend.h>
#include <Logme/Channel.h>
#include <Logme/Context.h>
#include <Logme/Logger.h>
#include <Logme/Override.h>
#include <Logme/SID.h>

#include <mutex>
#include <string>
#include <string.h>

namespace
{
  Logme::Level ToCppLevel(LogmeLevel level)
  {
    switch (level)
    {
      case Logme::LEVEL_DEBUG:
        return Logme::LEVEL_DEBUG;
      case Logme::LEVEL_INFO:
        return Logme::LEVEL_INFO;
      case Logme::LEVEL_WARN:
        return Logme::LEVEL_WARN;
      case Logme::LEVEL_ERROR:
        return Logme::LEVEL_ERROR;
      case Logme::LEVEL_CRITICAL:
        return Logme::LEVEL_CRITICAL;
      default:
        return Logme::LEVEL_INFO;
    }
  }

  const Logme::ID* BuildChannel(
    const char* channel
    , Logme::ID& storage
  )
  {
    if (channel == nullptr || channel[0] == '\0')
      return &CH;

    storage.Name = channel;
    return &storage;
  }

  const Logme::SID* BuildSubsystem(
    const char* subsystem
    , Logme::SID& storage
  )
  {
    if (subsystem == nullptr || subsystem[0] == '\0')
      return &SUBSID;

    storage = Logme::SID::Build(subsystem);
    return &storage;
  }

  void CopyError(
    char* errorBuffer
    , size_t errorBufferSize
    , const std::string& error
  )
  {
    if (errorBuffer == nullptr || errorBufferSize == 0)
      return;

    size_t count = error.size();
    if (count >= errorBufferSize)
      count = errorBufferSize - 1;

    if (count)
      memcpy(errorBuffer, error.c_str(), count);

    errorBuffer[count] = '\0';
  }


  std::mutex OverrideLock;

  void CopyToCppOverride(
    const LogmeCOverride& source
    , Logme::Override& target
  )
  {
    target.Add.Value = source.Add.Value;
    target.Remove.Value = source.Remove.Value;
    target.MaxRepetitions = source.MaxRepetitions;
    target.Repetitions = source.Repetitions;
    target.MaxFrequency = source.MaxFrequency;
    target.LastTime = source.LastTime;
    target.Shortener = (Logme::ShortenerPair*)source.Shortener;
  }

  void CopyFromCppOverride(
    const Logme::Override& source
    , LogmeCOverride& target
  )
  {
    target.Add.Value = source.Add.Value;
    target.Remove.Value = source.Remove.Value;
    target.MaxRepetitions = source.MaxRepetitions;
    target.Repetitions = source.Repetitions;
    target.MaxFrequency = source.MaxFrequency;
    target.LastTime = source.LastTime;
  }


  Logme::ChannelPtr GetOrCreateChannel(
    const char* channel
  )
  {
    if (Logme::Instance == nullptr)
      return Logme::ChannelPtr();

    Logme::ID id{ channel ? channel : "" };
    return Logme::Instance->GetChannel(id);
  }

  Logme::ChannelPtr GetExistingChannel(
    const char* channel
  )
  {
    if (Logme::Instance == nullptr)
      return Logme::ChannelPtr();

    Logme::ID id{ channel ? channel : "" };
    return Logme::Instance->GetExistingChannel(id);
  }

  void DoWriteV(
    LogmeLevel level
    , const char* channel
    , const char* subsystem
    , Logme::Override* overrideData
    , const char* function
    , const char* file
    , int line
    , const char* format
    , va_list args
  )
  {
    if (Logme::ShutdownCalled)
      return;

    if (Logme::Instance == nullptr)
      return;

    if (format == nullptr)
      format = "";

    Logme::ID channelStorage{ nullptr };
    Logme::SID subsystemStorage{ 0 };

    static Logme::ContextCache cache;
    Logme::Context context(
      cache
      , ToCppLevel(level)
      , BuildChannel(channel, channelStorage)
      , BuildSubsystem(subsystem, subsystemStorage)
      , function ? function : ""
      , file ? file : ""
      , line
      , Logme::Context::Params()
    );

    if (overrideData)
      context.Ovr = overrideData;

    Logme::Instance->DoLog(context, format, args);
  }
}

extern "C" void LogmeWriteV(
  LogmeLevel level
  , const char* channel
  , const char* subsystem
  , const char* function
  , const char* file
  , int line
  , const char* format
  , va_list args
)
{
  if (Logme::ShutdownCalled)
    return;

  if (Logme::Instance == nullptr)
    return;

  auto overrideData = Logme::Instance->GetThreadOverride();
  DoWriteV(
    level
    , channel
    , subsystem
    , &overrideData
    , function
    , file
    , line
    , format
    , args
  );
}

extern "C" void LogmeWriteOverrideV(
  LogmeLevel level
  , const char* channel
  , const char* subsystem
  , LogmeCOverride* overrideData
  , const char* function
  , const char* file
  , int line
  , const char* format
  , va_list args
)
{
  if (overrideData == nullptr)
  {
    LogmeWriteV(
      level
      , channel
      , subsystem
      , function
      , file
      , line
      , format
      , args
    );

    return;
  }

  std::lock_guard guard(OverrideLock);

  Logme::Override cppOverride;
  CopyToCppOverride(*overrideData, cppOverride);

  DoWriteV(
    level
    , channel
    , subsystem
    , &cppOverride
    , function
    , file
    , line
    , format
    , args
  );

  CopyFromCppOverride(cppOverride, *overrideData);
}

extern "C" void LogmeWrite(
  LogmeLevel level
  , const char* channel
  , const char* subsystem
  , const char* function
  , const char* file
  , int line
  , const char* format
  , ...
)
{
  va_list args;
  va_start(args, format);

  LogmeWriteV(
    level
    , channel
    , subsystem
    , function
    , file
    , line
    , format
    , args
  );

  va_end(args);
}

extern "C" void LogmeWriteOverride(
  LogmeLevel level
  , const char* channel
  , const char* subsystem
  , LogmeCOverride* overrideData
  , const char* function
  , const char* file
  , int line
  , const char* format
  , ...
)
{
  va_list args;
  va_start(args, format);

  LogmeWriteOverrideV(
    level
    , channel
    , subsystem
    , overrideData
    , function
    , file
    , line
    , format
    , args
  );

  va_end(args);
}

extern "C" void LogmeInitOverride(
  LogmeCOverride* overrideData
  , int maxRepetitions
  , uint64_t maxFrequency
)
{
  if (overrideData == nullptr)
    return;

  overrideData->Add.Value = 0;
  overrideData->Remove.Value = 0;
  overrideData->MaxRepetitions = maxRepetitions;
  overrideData->Repetitions = 0;
  overrideData->MaxFrequency = maxFrequency;
  overrideData->LastTime = 0;
  overrideData->Shortener = nullptr;
}

extern "C" int LogmeLoadConfigurationFile(
  const char* configFile
  , const char* section
  , char* errorBuffer
  , size_t errorBufferSize
)
{
  if (Logme::Instance == nullptr)
    return 0;

  std::string error;
  bool rc = Logme::Instance->LoadConfigurationFile(
    configFile ? configFile : ""
    , section ? section : ""
    , &error
  );

  CopyError(errorBuffer, errorBufferSize, error);
  return rc ? 1 : 0;
}

extern "C" int LogmeLoadConfiguration(
  const char* configData
  , const char* section
  , char* errorBuffer
  , size_t errorBufferSize
)
{
  if (Logme::Instance == nullptr)
    return 0;

  std::string error;
  bool rc = Logme::Instance->LoadConfiguration(
    configData ? configData : ""
    , section ? section : ""
    , &error
  );

  CopyError(errorBuffer, errorBufferSize, error);
  return rc ? 1 : 0;
}

extern "C" void LogmeShutdown(void)
{
  Logme::Logger::Shutdown();
}

extern "C" void LogmeSetChannelLevel(
  const char* channel
  , LogmeLevel level
)
{
  if (Logme::Instance == nullptr)
    return;

  Logme::ID id{ channel ? channel : "" };
  Logme::ChannelPtr ch = Logme::Instance->GetChannel(id);
  if (ch)
    ch->SetFilterLevel(ToCppLevel(level));
}

extern "C" void LogmeSetChannelEnabled(
  const char* channel
  , int enabled
)
{
  if (Logme::Instance == nullptr)
    return;

  Logme::ID id{ channel ? channel : "" };
  Logme::ChannelPtr ch = Logme::Instance->GetChannel(id);
  if (ch)
    ch->SetEnabled(enabled != 0);
}


extern "C" int LogmeCreateChannel(
  const char* channel
  , LogmeLevel level
)
{
  Logme::ChannelPtr ch = GetOrCreateChannel(channel);
  if (ch == nullptr)
    return 0;

  ch->SetFilterLevel(ToCppLevel(level));
  return 1;
}

extern "C" void LogmeDeleteChannel(
  const char* channel
)
{
  if (Logme::Instance == nullptr)
    return;

  Logme::ID id{ channel ? channel : "" };
  Logme::Instance->DeleteChannel(id);
}

extern "C" void LogmeRemoveChannelBackends(
  const char* channel
)
{
  Logme::ChannelPtr ch = GetExistingChannel(channel);
  if (ch)
    ch->RemoveBackends();
}

extern "C" void LogmeSetChannelFlags(
  const char* channel
  , LogmeOutputFlags flags
)
{
  Logme::ChannelPtr ch = GetOrCreateChannel(channel);
  if (ch == nullptr)
    return;

  ch->SetFlags(flags);
}

extern "C" int LogmeAddConsoleBackend(
  const char* channel
  , int async
)
{
  Logme::ChannelPtr ch = GetOrCreateChannel(channel);
  if (ch == nullptr)
    return 0;

  auto backend = std::make_shared<Logme::ConsoleBackend>(ch);
  backend->SetAsync(async != 0);
  ch->AddBackend(backend);
  return 1;
}

extern "C" int LogmeAddDebugBackend(
  const char* channel
)
{
  Logme::ChannelPtr ch = GetOrCreateChannel(channel);
  if (ch == nullptr)
    return 0;

  ch->AddBackend(std::make_shared<Logme::DebugBackend>(ch));
  return 1;
}

extern "C" int LogmeAddFileBackend(
  const char* channel
  , const char* fileName
  , int append
  , size_t maxSize
  , int dailyRotation
  , int maxParts
)
{
  if (fileName == nullptr || fileName[0] == '\0')
    return 0;

  Logme::ChannelPtr ch = GetOrCreateChannel(channel);
  if (ch == nullptr)
    return 0;

  auto backend = std::make_shared<Logme::FileBackend>(ch);
  auto config = std::make_shared<Logme::FileBackendConfig>();

  config->Filename = fileName;
  config->Append = append != 0;
  if (maxSize != 0)
    config->MaxSize = maxSize;

  config->DailyRotation = dailyRotation != 0;
  if (maxParts > 0)
    config->MaxParts = maxParts;

  if (!backend->ApplyConfig(config))
    return 0;

  ch->AddBackend(backend);
  return 1;
}

extern "C" void LogmeFlushChannel(
  const char* channel
)
{
  Logme::ChannelPtr ch = GetExistingChannel(channel);
  if (ch == nullptr)
    return;

  size_t count = ch->NumberOfBackends();
  for (size_t index = 0; index < count; ++index)
  {
    Logme::BackendPtr backend = ch->GetBackend(index);
    if (backend)
      backend->Flush();
  }
}


extern "C" void LogmeFlushAll(void)
{
  if (Logme::Instance)
    Logme::Instance->FlushAll();
}
