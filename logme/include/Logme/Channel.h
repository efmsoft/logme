#pragma once

#include <Logme/Backend/Backend.h>
#include <Logme/Context.h>
#include <Logme/OutputFlags.h>
#include <Logme/Types.h>

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>

namespace Logme
{
  struct ChannelConfig
  {
    std::string Name;
    OutputFlags Flags;
    Level Filter;
    BackendConfigArray Backend;
    std::optional<std::string> Link;

    LOGMELNK ChannelConfig() : Filter(DEFAULT_LEVEL) {}
  };

  typedef std::vector<ChannelConfig> ChannelConfigArray;

  class Channel
  {
    std::mutex DataLock;

    class Logger* Owner;
    std::string Name;
    OutputFlags Flags;
    Level LevelFilter;

    BackendArray Backends;
    uint64_t AccessCount;

    IDPtr Link;
    ChannelPtr LinkTo;

    std::map<std::string, std::string> ShortenerMap;
    std::map<uint64_t, std::string> ThreadName;

  public:
    LOGMELNK Channel(
      Logger* owner
      , const char* name
      , const OutputFlags& flags
      , Level level
    );
    LOGMELNK ~Channel();

    LOGMELNK bool operator==(const char* name) const;
    LOGMELNK void Display(Context& context, const char* line);

    LOGMELNK const OutputFlags& GetFlags() const;
    LOGMELNK void SetFlags(const OutputFlags& flags);

    LOGMELNK Level GetFilterLevel() const;
    LOGMELNK void SetFilterLevel(Level level);

    LOGMELNK void AddBackend(BackendPtr backend);
    LOGMELNK bool RemoveBackend(BackendPtr backend);
    LOGMELNK void RemoveBackends();
    
    LOGMELNK BackendPtr GetBackend(size_t index);
    LOGMELNK size_t NumberOfBackends();

    LOGMELNK BackendPtr FindFirstBackend(const char* type, int& context);
    LOGMELNK BackendPtr FindNextBackend(const char* type, int& context);

    LOGMELNK Logger* GetOwner() const;
    LOGMELNK std::string GetName();
    LOGMELNK uint64_t GetAccessCount() const;

    LOGMELNK void AddLink(const ID& to);
    LOGMELNK void AddLink(ChannelPtr to);
    LOGMELNK void RemoveLink();
    LOGMELNK bool IsLinked() const;
    LOGMELNK ChannelPtr GetLinkPtr();

    LOGMELNK const ID GetID() const;
    
    LOGMELNK void Freeze();
    LOGMELNK bool IsIdle();

    LOGMELNK void ShortenerAdd(const char* what, const char* replace_on);
    LOGMELNK const char* ShortenerRun(const char* value, ShortenerContext& context);

    LOGMELNK void SetThreadName(uint64_t id, const char* name);
    LOGMELNK const char* GetThreadName(uint64_t id);
  };

  typedef std::shared_ptr<Channel> ChannelPtr;
  typedef std::vector<ChannelPtr> ChannelArray;
  typedef std::unordered_map<std::string, ChannelPtr> ChannelMap;
}
