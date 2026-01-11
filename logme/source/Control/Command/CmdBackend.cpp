#include <string>
#include <algorithm>
#include <cctype>


#include <Logme/Backend/Backend.h>
#include <Logme/Logger.h>

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

  return std::string();
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
    int ctx = 0;
    if (ch->FindFirstBackend(type.c_str(), ctx))
    {
      response = "error: backend already exists: " + typeInput;
      return true;
    }

    auto backend = Backend::Create(type.c_str(), ch);
    if (!backend)
    {
      response = "error: unknown backend type: " + typeInput;
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
