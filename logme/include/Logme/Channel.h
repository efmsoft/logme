#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>

#include <Logme/Backend/Backend.h>
#include <Logme/CritSection.h>
#include <Logme/Context.h>
#include <Logme/OutputFlags.h>
#include <Logme/Override.h>
#include <Logme/SafeID.h>
#include <Logme/Types.h>

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
    mutable CS DataLock;

    class Logger* Owner;
    std::string Name;
    SafeID ChannelID;
    OutputFlags Flags;
    std::atomic<Level> LevelFilter;
    std::atomic<bool> Enabled;
    std::atomic<bool> Linked;
    std::atomic<size_t> BackendCount;
    std::atomic<bool> Active;

    void UpdateActive();

    BackendArray Backends;
    std::atomic<uint64_t> AccessCount;

    IDPtr Link;
    ChannelPtr LinkTo;

    CS ShortenerLock;
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
    /// <summary>
    /// Creates channel with initial output flags and level filter.
    /// </summary>
    /// <param name="owner">Logger that owns this channel.</param>
    /// <param name="name">Channel name. nullptr creates default unnamed channel.</param>
    /// <param name="flags">Initial output flags applied before per-record override.</param>
    /// <param name="level">Minimum level accepted by channel.</param>
    LOGMELNK Channel(
      Logger* owner
      , const char* name
      , const OutputFlags& flags
      , Level level
    );

    /// <summary>
    /// Destroys channel and releases backend references.
    /// </summary>
    LOGMELNK ~Channel();

    /// <summary>
    /// Compares channel name with specified text.
    /// </summary>
    /// <param name="name">Channel name to compare. Must not be nullptr.</param>
    /// <returns>true if channel name matches.</returns>
    LOGMELNK bool operator==(const char* name) const;

    /// <summary>
    /// Sends prepared log context to this channel backends and linked channel.
    /// </summary>
    /// <param name="context">Prepared log context. Display filter and output flags may use and modify it.</param>
    LOGMELNK void Display(Context& context);

    /// <summary>
    /// Sets optional filter called before channel writes a record.
    /// </summary>
    /// <param name="filter">Filter callback; empty callback disables filtering.</param>
    LOGMELNK void SetDisplayFilter(TDisplayFilter filter = TDisplayFilter());

    /// <summary>
    /// Returns channel output flags.
    /// </summary>
    /// <returns>Current channel output flags.</returns>
    LOGMELNK const OutputFlags& GetFlags() const;

    /// <summary>
    /// Replaces channel output flags.
    /// </summary>
    /// <param name="flags">New output flags used for following records.</param>
    LOGMELNK void SetFlags(const OutputFlags& flags);

    /// <summary>
    /// Enables or disables channel output.
    /// </summary>
    /// <param name="enable">false prevents writing to channel backends.</param>
    LOGMELNK void SetEnabled(bool enable);

    /// <summary>
    /// Returns channel enabled flag.
    /// </summary>
    /// <returns>true if channel output is enabled.</returns>
    LOGMELNK bool GetEnabled() const;

    /// <summary>
    /// Returns minimum level accepted by channel.
    /// </summary>
    /// <returns>Current channel level filter.</returns>
    LOGMELNK Level GetFilterLevel() const;

    /// <summary>
    /// Sets minimum level accepted by channel.
    /// </summary>
    /// <param name="level">Records below this level are ignored by channel.</param>
    LOGMELNK void SetFilterLevel(Level level);

    /// <summary>
    /// Checks whether channel can currently receive output or forward it through link.
    /// </summary>
    /// <returns>true when channel is linked or has enabled output with at least one backend.</returns>
    LOGMELNK bool GetActive() const;

    /// <summary>
    /// Adds backend to channel if it is not already present.
    /// </summary>
    /// <param name="backend">Backend object to add. Must not be nullptr.</param>
    LOGMELNK void AddBackend(BackendPtr backend);

    /// <summary>
    /// Removes backend from channel and freezes it.
    /// </summary>
    /// <param name="backend">Backend object to remove.</param>
    /// <returns>true if backend was found and removed.</returns>
    LOGMELNK bool RemoveBackend(BackendPtr backend);

    /// <summary>
    /// Freezes and removes all backends from channel.
    /// </summary>
    LOGMELNK void RemoveBackends();

    /// <summary>
    /// Returns backend by index.
    /// </summary>
    /// <param name="index">Zero-based backend index.</param>
    /// <returns>Backend pointer, or nullptr when index is out of range.</returns>
    LOGMELNK BackendPtr GetBackend(size_t index);

    /// <summary>
    /// Returns number of backends currently attached to channel.
    /// </summary>
    /// <returns>Backend count.</returns>
    LOGMELNK size_t NumberOfBackends() const;

    /// <summary>
    /// Starts search for backend by type.
    /// </summary>
    /// <param name="type">Backend type id to find.</param>
    /// <param name="context">Search position storage initialized by this call.</param>
    /// <returns>First backend with matching type, or nullptr when not found.</returns>
    LOGMELNK BackendPtr FindFirstBackend(const char* type, int& context);

    /// <summary>
    /// Continues search for backend by type.
    /// </summary>
    /// <param name="type">Backend type id to find.</param>
    /// <param name="context">Search position returned by previous FindFirstBackend or FindNextBackend call.</param>
    /// <returns>Next backend with matching type, or nullptr when not found.</returns>
    LOGMELNK BackendPtr FindNextBackend(const char* type, int& context);

    /// <summary>
    /// Returns logger that owns this channel.
    /// </summary>
    /// <returns>Owner logger pointer.</returns>
    LOGMELNK Logger* GetOwner() const;

    /// <summary>
    /// Returns channel name.
    /// </summary>
    /// <returns>Channel name. Default channel name is empty string.</returns>
    LOGMELNK std::string GetName();

    /// <summary>
    /// Returns number of records passed to Display.
    /// </summary>
    /// <returns>Access counter value.</returns>
    LOGMELNK uint64_t GetAccessCount() const;

    /// <summary>
    /// Links this channel to channel id.
    /// </summary>
    /// <param name="to">Destination channel id. Destination is resolved when record is displayed.</param>
    LOGMELNK void AddLink(const ID& to);

    /// <summary>
    /// Links this channel to channel object.
    /// </summary>
    /// <param name="to">Destination channel pointer.</param>
    LOGMELNK void AddLink(const ChannelPtr& to);

    /// <summary>
    /// Removes channel link.
    /// </summary>
    LOGMELNK void RemoveLink();

    /// <summary>
    /// Checks whether channel has configured link.
    /// </summary>
    /// <returns>true if channel is linked to another channel.</returns>
    LOGMELNK bool IsLinked() const;

    /// <summary>
    /// Checks whether channel would accept record with specified context.
    /// </summary>
    /// <param name="context">Log context with level used for level filter check.</param>
    /// <returns>true if context level passes filter and channel is active.</returns>
    LOGMELNK bool IsOutputActive(const Context& context) const;

    /// <summary>
    /// Returns object link target when link was set by ChannelPtr.
    /// </summary>
    /// <returns>Linked channel pointer, or nullptr.</returns>
    LOGMELNK ChannelPtr GetLinkPtr();

    /// <summary>
    /// Returns channel id.
    /// </summary>
    /// <returns>Stable channel id created from channel name.</returns>
    LOGMELNK const ID& GetID() const;

    /// <summary>
    /// Freezes all channel backends so they stop receiving new records.
    /// </summary>
    LOGMELNK void Freeze();

    /// <summary>
    /// Checks whether all backends are idle.
    /// </summary>
    /// <returns>true if every backend reports idle state.</returns>
    LOGMELNK bool IsIdle();

    /// <summary>
    /// Flushes all channel backends.
    /// </summary>
    LOGMELNK void Flush();

    /// <summary>
    /// Returns channel data lock.
    /// </summary>
    /// <returns>Internal channel lock used by advanced integration code.</returns>
    LOGMELNK CS& GetDataLock();

    /// <summary>
    /// Adds, replaces or removes prefix shortener rule.
    /// </summary>
    /// <param name="what">Prefix to search at beginning of value.</param>
    /// <param name="replace_on">Replacement prefix, or nullptr to remove rule.</param>
    LOGMELNK void ShortenerAdd(const char* what, const char* replace_on);

    /// <summary>
    /// Sets static prefix shortener table.
    /// </summary>
    /// <param name="pair">Null-terminated shortener pair table, or nullptr to disable table.</param>
    LOGMELNK void SetShortenerPair(const ShortenerPair* pair);

    /// <summary>
    /// Applies channel shortener rules to value.
    /// </summary>
    /// <param name="value">Input text. Must remain valid while returned pointer is used.</param>
    /// <param name="context">Scratch storage used when shortened value cannot point into input.</param>
    /// <returns>Original value, pointer inside original value, or pointer to context storage.</returns>
    LOGMELNK const char* ShortenerRun(const char* value, ShortenerContext& context);

    /// <summary>
    /// Applies override shortener rules to value.
    /// </summary>
    /// <param name="value">Input text. Must remain valid while returned pointer is used.</param>
    /// <param name="context">Scratch storage used when shortened value cannot point into input.</param>
    /// <param name="ovr">Override containing shortener table.</param>
    /// <returns>Original value, pointer inside original value, or pointer to context storage.</returns>
    LOGMELNK const char* ShortenerRun(const char* value, ShortenerContext& context, Override& ovr);

    /// <summary>
    /// Applies specified shortener pair table to value.
    /// </summary>
    /// <param name="value">Input text. Must remain valid while returned pointer is used.</param>
    /// <param name="context">Scratch storage used when shortened value cannot point into input.</param>
    /// <param name="pair">Null-terminated shortener pair table.</param>
    /// <returns>Original value, pointer inside original value, or pointer to context storage.</returns>
    LOGMELNK const char* ShortenerPairRun(const char* value, ShortenerContext& context, const ShortenerPair* pair);

    /// <summary>
    /// Sets, clears or updates display name for thread id.
    /// </summary>
    /// <param name="id">Thread id.</param>
    /// <param name="name">Thread display name, or nullptr to clear name.</param>
    /// <param name="log">true to remember previous name for transition output.</param>
    LOGMELNK void SetThreadName(uint64_t id, const char* name, bool log = true);

    struct ThreadNameInfo
    {
      std::string Name;
      char Buffer[32]{};
    };

    /// <summary>
    /// Returns display name for thread id.
    /// </summary>
    /// <param name="id">Thread id.</param>
    /// <param name="info">Storage used when name must be copied before returning.</param>
    /// <param name="transition">Optional output receiving previous thread name.</param>
    /// <param name="clear">true to clear stored transition after reading it.</param>
    /// <returns>Thread display name, or nullptr when no name is available.</returns>
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
