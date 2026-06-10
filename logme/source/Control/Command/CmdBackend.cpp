#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <limits>
#include <string>

#include <Logme/Backend/Backend.h>
#include <Logme/Backend/BufferBackend.h>
#include <Logme/Backend/ConsoleBackend.h>
#include <Logme/Backend/FileBackend.h>
#include <Logme/Backend/RingBufferBackend.h>
#include <Logme/Backend/SharedFileBackend.h>
#include <Logme/Logger.h>
#include <Logme/Utils.h>

#include "../CommandRegistrar.h"

using namespace Logme;

COMMAND_DESCRIPTOR2("backend", Logger::CommandBackend);

static std::string NormalizeBackendType(const std::string& input)
{
  std::string s = input;
  std::transform(
    s.begin()
    , s.end()
    , s.begin()
    , [](unsigned char c)
    {
      return static_cast<char>(std::tolower(c));
    }
  );

  // Accept "ConsoleBackend", "consolebackend", "console", "con", etc.
  if (s.size() >= 7 && s.rfind("backend") == s.size() - 7)
    s.resize(s.size() - 7);

  if (s == "console" || s == "con")
    return "ConsoleBackend";

  if (s == "debug" || s == "dbg")
    return "DebugBackend";

  if (s == "file")
    return "FileBackend";

  if (s == "sharedfile" || s == "shared" || s == "sfile")
    return "SharedFileBackend";

  if (s == "buffer" || s == "buf")
    return "BufferBackend";

  if (s == "ringbuffer" || s == "ring" || s == "rbuf")
    return "RingBufferBackend";

  // If caller already passed exact canonical name with case variants, map it too.
  if (s == "consolebackend")
    return "ConsoleBackend";
  if (s == "debugbackend")
    return "DebugBackend";
  if (s == "filebackend")
    return "FileBackend";
  if (s == "sharedfilebackend")
    return "SharedFileBackend";
  if (s == "bufferbackend")
    return "BufferBackend";
  if (s == "ringbufferbackend")
    return "RingBufferBackend";

  return std::string();
}



static bool ParseIntValue(const std::string& input, int& value)
{
  if (input.empty())
    return false;

  try
  {
    size_t pos = 0;
    int parsed = std::stoi(input, &pos);
    if (pos != input.size())
      return false;

    value = parsed;
    return true;
  }
  catch (...)
  {
    return false;
  }
}

static bool ParseBufferPolicy(const std::string& input, BufferBackendPolicy& policy)
{
  std::string s = input;
  ToLowerAsciiInplace(s);

  if (s == "stop" || s == "stop-appending" || s == "stop_appending")
  {
    policy = BufferBackendPolicy::STOP_APPENDING;
    return true;
  }

  if (s == "delete" || s == "delete-oldest" || s == "delete_oldest")
  {
    policy = BufferBackendPolicy::DELETE_OLDEST;
    return true;
  }

  return false;
}

static bool GetChannelFromArgs(
  Logme::StringArray& arr
  , size_t& index
  , ChannelPtr& ch
  , std::string& error
)
{
  if (index + 1 < arr.size() && arr[index] == "--channel")
  {
    const std::string& name = arr[index + 1];
    if (name.empty())
    {
      error = "invalid channel name";
      return false;
    }

    ch = Logme::Instance->GetExistingChannel(ID{name.c_str()});
    if (ch == nullptr)
    {
      error = "no such channel: " + name;
      return false;
    }

    index += 2;
    return true;
  }

  ch = Logme::Instance->GetExistingChannel(::CH);
  if (ch == nullptr)
  {
    error = "no default channel";
    return false;
  }

  return true;
}

bool Logger::CommandBackend(Logme::StringArray& arr, std::string& response)
{
  size_t index = 1;
  ChannelPtr ch;
  std::string error;

  std::lock_guard guard(Instance->DataLock);

  if (!GetChannelFromArgs(arr, index, ch, error))
  {
    response = "error: " + error;
    return true;
  }

  if (index + 2 > arr.size())
  {
    response = "error: missing operation";
    return true;
  }

  const std::string& op = arr[index];
  const std::string& typeInput = arr[index + 1];
  const std::string type = NormalizeBackendType(typeInput);

  if (op != "--add" && op != "--delete")
  {
    response = "error: unknown option: " + op;
    return true;
  }

  if (typeInput.empty())
  {
    response = "error: invalid backend type";
    return true;
  }

  if (type.empty())
  {
    response = "error: unknown backend type: " + typeInput;
    return true;
  }

  if (op == "--add")
  {
    if (type != FileBackend::TYPE_ID && type != SharedFileBackend::TYPE_ID)
    {
      int ctx = 0;
      if (ch->FindFirstBackend(type.c_str(), ctx))
      {
        response = "error: backend already exists: " + typeInput;
        return true;
      }
    }

    auto backend = Backend::Create(type.c_str(), ch);
    if (!backend)
    {
      response = "error: unknown backend type: " + typeInput;
      return true;
    }

    BackendConfigPtr config = backend->CreateConfig();

    for (size_t i = index + 2; i < arr.size(); ++i)
    {
      const std::string& arg = arr[i];

      if (arg == "--async")
      {
        if (backend->IsAsyncSupported())
        {
          config->Async = true;
          continue;
        }

        response = "error: --async is not supported by this backend";
        return true;
      }

      if (arg == "--file")
      {
        if (i + 1 >= arr.size())
        {
          response = "error: missing file name";
          return true;
        }

        auto fileConfig = std::dynamic_pointer_cast<FileBackendConfig>(config);
        auto sharedFileConfig = std::dynamic_pointer_cast<SharedFileBackendConfig>(config);

        if (fileConfig)
        {
          fileConfig->Filename = arr[++i];
          continue;
        }

        if (sharedFileConfig)
        {
          sharedFileConfig->Filename = arr[++i];
          continue;
        }

        response = "error: --file is only supported by file backends";
        return true;
      }

      if (arg == "--append" || arg == "--overwrite")
      {
        auto fileConfig = std::dynamic_pointer_cast<FileBackendConfig>(config);
        if (fileConfig)
        {
          fileConfig->Append = (arg == "--append");
          continue;
        }

        response = "error: " + arg + " is only supported by FileBackend";
        return true;
      }

      if (arg == "--max-size")
      {
        if (i + 1 >= arr.size())
        {
          response = "error: missing max size";
          return true;
        }

        uint64_t value = 0;
        if (ParseByteSize(arr[++i], value) == false
          || value > std::numeric_limits<size_t>::max())
        {
          response = "error: invalid max size";
          return true;
        }

        auto fileConfig = std::dynamic_pointer_cast<FileBackendConfig>(config);
        auto sharedFileConfig = std::dynamic_pointer_cast<SharedFileBackendConfig>(config);
        auto bufferConfig = std::dynamic_pointer_cast<BufferBackendConfig>(config);

        if (fileConfig)
        {
          fileConfig->MaxSize = static_cast<size_t>(value);
          continue;
        }

        if (sharedFileConfig)
        {
          sharedFileConfig->MaxSize = static_cast<size_t>(value);
          continue;
        }

        if (bufferConfig)
        {
          bufferConfig->MaxSize = static_cast<size_t>(value);
          continue;
        }

        response = "error: --max-size is only supported by file and buffer backends";
        return true;
      }

      if (arg == "--daily-rotation" || arg == "--no-daily-rotation")
      {
        auto fileConfig = std::dynamic_pointer_cast<FileBackendConfig>(config);
        if (fileConfig)
        {
          fileConfig->DailyRotation = (arg == "--daily-rotation");
          continue;
        }

        response = "error: " + arg + " is only supported by FileBackend";
        return true;
      }

      if (arg == "--max-parts")
      {
        if (i + 1 >= arr.size())
        {
          response = "error: missing max parts";
          return true;
        }

        int value = 0;
        if (!ParseIntValue(arr[++i], value))
        {
          response = "error: invalid max parts";
          return true;
        }

        auto fileConfig = std::dynamic_pointer_cast<FileBackendConfig>(config);
        if (fileConfig)
        {
          fileConfig->MaxParts = value;
          continue;
        }

        response = "error: --max-parts is only supported by FileBackend";
        return true;
      }

      if (arg == "--timeout")
      {
        if (i + 1 >= arr.size())
        {
          response = "error: missing timeout";
          return true;
        }

        uint64_t value = 0;
        if (ParseInterval(arr[++i], value) == false
          || value > std::numeric_limits<size_t>::max())
        {
          response = "error: invalid timeout";
          return true;
        }

        auto sharedFileConfig = std::dynamic_pointer_cast<SharedFileBackendConfig>(config);
        if (sharedFileConfig)
        {
          sharedFileConfig->Timeout = static_cast<size_t>(value);
          continue;
        }

        response = "error: --timeout is only supported by SharedFileBackend";
        return true;
      }

      if (arg == "--policy")
      {
        if (i + 1 >= arr.size())
        {
          response = "error: missing buffer policy";
          return true;
        }

        BufferBackendPolicy policy;
        if (!ParseBufferPolicy(arr[++i], policy))
        {
          response = "error: invalid buffer policy";
          return true;
        }

        auto bufferConfig = std::dynamic_pointer_cast<BufferBackendConfig>(config);
        if (bufferConfig)
        {
          bufferConfig->Policy = policy;
          continue;
        }

        response = "error: --policy is only supported by BufferBackend";
        return true;
      }

      if (arg == "--max-items")
      {
        if (i + 1 >= arr.size())
        {
          response = "error: missing max items";
          return true;
        }

        int value = 0;
        if (!ParseIntValue(arr[++i], value) || value <= 0)
        {
          response = "error: invalid max items";
          return true;
        }

        auto ringBufferConfig = std::dynamic_pointer_cast<RingBufferBackendConfig>(config);
        if (ringBufferConfig)
        {
          ringBufferConfig->MaxItems = static_cast<size_t>(value);
          continue;
        }

        response = "error: --max-items is only supported by RingBufferBackend";
        return true;
      }

      response = "error: unknown backend option: " + arg;
      return true;
    }

    if (config && !backend->ApplyConfig(config))
    {
      response = "error: failed to apply backend configuration";
      return true;
    }

    ch->AddBackend(backend);
    response = "ok";
    return true;
  }

  int ctx = 0;
  auto backend = ch->FindFirstBackend(type.c_str(), ctx);
  if (!backend)
  {
    response = "error: no such backend: " + typeInput;
    return true;
  }

  if (!ch->RemoveBackend(backend))
  {
    response = "error: failed to remove backend: " + typeInput;
    return true;
  }

  response = "ok";
  return true;
}
