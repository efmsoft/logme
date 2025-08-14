#pragma once

#include <Logme/Backend/Backend.h>
#include <Logme/Context.h>
#include <Logme/Override.h>
#include <Logme/OutputFlags.h>
#include <Logme/Types.h>

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>

namespace Logme
{
  typedef std::shared_ptr<class Channel> ChannelPtr;

  struct ChannelConfig
  {
    bool Enabled;
    std::string Name;
    OutputFlags Flags;
    Level Filter;
    BackendConfigArray Backend;
    std::optional<std::string> Link;

    ChannelPtr Object;

    LOGMELNK ChannelConfig() 
      : Enabled(true)
      , Filter(DEFAULT_LEVEL) 
    {
    }
  };

  typedef std::vector<ChannelConfig> ChannelConfigArray;
  typedef std::function<bool(Context&, const char*)> TDisplayFilter;

  class Channel : public std::enable_shared_from_this<Channel>
  {
    std::recursive_mutex DataLock;

    class Logger* Owner;
    std::string Name;
    ID ChannelID;
    OutputFlags Flags;
    Level LevelFilter;
    bool Enabled;

    BackendArray Backends;
    uint64_t AccessCount;

    IDPtr Link;
    ChannelPtr LinkTo;

    const ShortenerPair* ShortenerList;
    std::map<std::string, std::string> ShortenerMap;

    struct ThreadNameRecord
    {
      std::optional<std::string> Name;
      std::optional<std::string> Prev;
    };
    std::map<uint64_t, ThreadNameRecord> ThreadName;

    TDisplayFilter DisplayFilter;

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
    LOGMELNK void SetDisplayFilter(TDisplayFilter filter = TDisplayFilter());

    LOGMELNK const OutputFlags& GetFlags() const;
    LOGMELNK void SetFlags(const OutputFlags& flags);

    LOGMELNK void SetEnabled(bool enable);
    LOGMELNK bool GetEnabled() const;

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

    LOGMELNK const ID& GetID() const;
    
    LOGMELNK void Freeze();
    LOGMELNK bool IsIdle();
    LOGMELNK void Flush();

    LOGMELNK void ShortenerAdd(const char* what, const char* replace_on);
    LOGMELNK void SetShortenerPair(const ShortenerPair* pair);
    LOGMELNK const char* ShortenerRun(const char* value, ShortenerContext& context);
    LOGMELNK const char* ShortenerRun(const char* value, ShortenerContext& context, Override& ovr);
    LOGMELNK const char* ShortenerPairRun(const char* value, ShortenerContext& context, const ShortenerPair* pair);

    LOGMELNK void SetThreadName(uint64_t id, const char* name, bool log = true);

    struct ThreadNameInfo
    {
      std::string Name;
      char Buffer[32]{};
    };

    LOGMELNK const char* GetThreadName(
      uint64_t id
      , ThreadNameInfo& info
      , std::optional<std::string>* transition = nullptr
      , bool clear = true
    );
  };

  typedef std::vector<ChannelPtr> ChannelArray;
  typedef std::unordered_map<std::string, ChannelPtr> ChannelMap;
}
