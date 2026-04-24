#pragma once

#ifndef LOGME_DISABLE_STD_FORMAT
#include <format>
#endif
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <Logme/Channel.h>
#include <Logme/CritSection.h>
#include <Logme/File/DirectorySizeWatchdog.h>
#include <Logme/File/FileManagerFactory.h>
#include <Logme/Obfuscate.h>
#include <Logme/Stream.h>
#include <Logme/Utils.h>

namespace Logme
{
  typedef std::shared_ptr<std::string> StringPtr;
  typedef std::function<bool(const std::string&, std::string&)> TControlHandler;
  typedef bool (*TCondition)();
  typedef std::function<void(const std::string&, const ChannelPtr&)> TChannelCallback;

#ifndef LOGME_DISABLE_STD_FORMAT
  struct StdFormat{};
#endif
  struct ControlSslContext;
  LOGMELNK extern bool ShutdownCalled;

  class Logger : public std::enable_shared_from_this<Logger>
  {
    CS DataLock;

    ChannelMap Channels;
    ChannelPtr Default;
    
    std::string HomeDirectory;
    DirectorySizeWatchdog HomeDirectoryWatchDog;

    bool BlockReportedSubsystems;
    std::vector<uint64_t> Subsystems;

    int IDGenerator;

    FileManagerFactory Factory;

    std::mutex ErrorLock;
    StringPtr ErrorChannel;

    int ControlSocket;
    ControlConfig ControlCfg;
    std::string ControlPassword;
    TControlHandler ControlExtension;
    ControlSslContext* ControlSsl;
    
    typedef std::shared_ptr<std::thread> ThreadPtr;
    ThreadPtr ListenerThread;

    ChannelArray ToDelete;
    unsigned LastDoAutodelete;
    int NumDeleting;

    bool EnableVTMode;

    struct ControlThread
    {
      ThreadPtr Thread;
      bool Stopped;
      bool Authorized;

      ControlThread()
        : Stopped(false)
        , Authorized(false)
      {
      }
    };

    typedef std::map<int, ControlThread> ThreadMap;
    ThreadMap ControlThreads;

    ObfKey Key;
    bool Obfuscate;

  public:
    TCondition Condition;

  public:
    /// <summary>
    /// Creates logger with default channel layout and home directory set to executable path.
    /// </summary>
    LOGMELNK Logger();

    /// <summary>
    /// Stops control server, releases SSL context and deletes all channels.
    /// </summary>
    LOGMELNK virtual ~Logger();

    /// <summary>
    /// Stops global logger instance and disables further logging calls.
    /// </summary>
    LOGMELNK static void Shutdown();

    /// <summary>
    /// Sets or clears default channel for current thread.
    /// </summary>
    /// <param name="id">Channel id used by this thread, or nullptr to clear thread channel.</param>
    LOGMELNK void SetThreadChannel(const ID* id);

    /// <summary>
    /// Checks whether current thread has its own default channel.
    /// </summary>
    /// <returns>true if SetThreadChannel assigned channel for current thread.</returns>
    LOGMELNK bool IsChannelDefinedForCurrentThread();

    /// <summary>
    /// Returns current thread channel id or global default channel id.
    /// </summary>
    /// <returns>Thread-local channel id when set; otherwise global CH.</returns>
    LOGMELNK SafeID GetDefaultChannel();

    /// <summary>
    /// Returns default channel object.
    /// </summary>
    /// <returns>Default channel pointer, or nullptr after shutdown.</returns>
    LOGMELNK ChannelPtr GetDefaultChannelPtr();

    /// <summary>
    /// Sets or clears output override for current thread.
    /// </summary>
    /// <param name="ovr">Override copied into thread-local context, or nullptr to clear it.</param>
    LOGMELNK void SetThreadOverride(const Override* ovr);

    /// <summary>
    /// Returns output override assigned to current thread.
    /// </summary>
    /// <returns>Thread-local override, or default Override when none is set.</returns>
    LOGMELNK Override GetThreadOverride();

    /// <summary>
    /// Adds subsystem id to reported subsystem list.
    /// </summary>
    /// <param name="sid">Subsystem id to report. Empty SID is ignored.</param>
    LOGMELNK void ReportSubsystem(const SID& sid);

    /// <summary>
    /// Removes subsystem id from reported subsystem list.
    /// </summary>
    /// <param name="sid">Subsystem id to remove. Empty SID is ignored.</param>
    LOGMELNK void UnreportSubsystem(const SID& sid);

    /// <summary>
    /// Selects whether reported subsystems are blocked or exclusively allowed.
    /// </summary>
    /// <param name="block">true to suppress reported subsystems; false to log only reported subsystems.</param>
    LOGMELNK void SetBlockReportedSubsystems(bool block);

    /// <summary>
    /// Loads logger configuration from UTF-16 file path.
    /// </summary>
    /// <param name="config_file">Path to configuration file.</param>
    /// <param name="section">Configuration section name; empty string selects default section.</param>
    /// <returns>true if configuration was loaded and applied.</returns>
    LOGMELNK bool LoadConfigurationFile(const std::wstring& config_file, const std::string& section = std::string());

    /// <summary>
    /// Loads logger configuration from file.
    /// </summary>
    /// <param name="config_file">Path to configuration file.</param>
    /// <param name="section">Configuration section name; empty string selects default section.</param>
    /// <returns>true if configuration was loaded and applied.</returns>
    LOGMELNK bool LoadConfigurationFile(const std::string& config_file, const std::string& section = std::string());

    /// <summary>
    /// Loads logger configuration from text buffer.
    /// </summary>
    /// <param name="config_data">Configuration text.</param>
    /// <param name="section">Configuration section name; empty string selects default section.</param>
    /// <returns>true if configuration was parsed and applied.</returns>
    LOGMELNK bool LoadConfiguration(const std::string& config_data, const std::string& section = std::string());

    /// <summary>
    /// Formats message and sends prepared context to target channel.
    /// </summary>
    /// <param name="context">Log context with level, source location, channel and override data.</param>
    /// <param name="format">printf-style format string. Special "%s" path uses supplied text directly.</param>
    /// <param name="args">Arguments for format.</param>
    LOGMELNK virtual void DoLog(Context& context, const char* format, va_list args);

    /// <summary>
    /// Writes a log record. Overloads accept optional channel/id, subsystem id and override
    /// in different argument orders for macro and direct API compatibility.
    /// </summary>
    /// <remarks>
    /// Stream overloads return a stream sink. Variadic overloads use printf-style formatting.
    /// std::format overloads are selected by passing Logme::GetStdFormat().
    /// </remarks>
    /// <param name="context">Prepared log context.</param>
    /// <param name="id">Channel id.</param>
    /// <param name="ch">Channel pointer.</param>
    /// <param name="sid">Subsystem id.</param>
    /// <param name="ovr">Per-call output override.</param>
    /// <param name="format">printf-style format string.</param>
    /// <param name="fmt">std::format-style format string.</param>

    LOGMELNK Stream Log(const Context& context); // @1

    LOGMELNK Stream Log(const Context& context, Override& ovr); // @2
    LOGMELNK Stream Log(const Context& context, Override& ovr, const SID& sid);
    LOGMELNK Stream Log(const Context& context, const SID& sid, Override& ovr); // @3
    LOGMELNK Stream Log(const Context& context, const ID& id); // @4
    LOGMELNK Stream Log(const Context& context, const ChannelPtr& ch); // @5
    LOGMELNK Stream Log(const Context& context, const ID& id, const SID& sid); // @6
    LOGMELNK Stream Log(const Context& context, const ChannelPtr& ch, const SID& sid); // @7
    LOGMELNK Stream Log(const Context& context, Override& ovr, const ID& id);
    LOGMELNK Stream Log(const Context& context, Override& ovr, const ChannelPtr& ch);
    LOGMELNK Stream Log(const Context& context, const ID& id, Override& ovr); // @8
    LOGMELNK Stream Log(const Context& context, const ChannelPtr& ch, Override& ovr); // @9
    LOGMELNK Stream Log(const Context& context, Override& ovr, const ID& id, const SID& sid);
    LOGMELNK Stream Log(const Context& context, Override& ovr, const ChannelPtr& ch, const SID& sid);
    LOGMELNK Stream Log(const Context& context, const ID& id, const SID& sid, Override& ovr); // @10
    LOGMELNK Stream Log(const Context& context, const ChannelPtr& ch, const SID& sid, Override& ovr); // @11

#ifndef LOGME_DISABLE_STD_FORMAT
    template<typename... Args>
    void Log(const Context& context, const StdFormat*, const ID& id, const char* fmt, Args&&... args)
    {
      if (ShutdownCalled)
        return;

      std::string out = std::vformat(fmt, std::make_format_args(args...));
      Log(context, id, "%s", out.c_str());
    }

    template<typename... Args>
    void Log(const Context& context, const StdFormat*, const ChannelPtr& ch, const char* fmt, Args&&... args)
    {
      if (ShutdownCalled)
        return;

      if (ch && context.ErrorLevel < Level::LEVEL_ERROR && ch->IsOutputActive(context) == false)
        return;

      std::string out = std::vformat(fmt, std::make_format_args(args...));
      Log(context, ch, "%s", out.c_str());
    }
#endif
    LOGMELNK void Log(const Context& context, const ID& id, const char* format, ...);
    LOGMELNK void Log(const Context& context, const ChannelPtr& ch, const char* format, ...);
    
    LOGMELNK void Log(const Context& context, const ID& id,  const SID& sid, const char* format, ...);
    LOGMELNK void Log(const Context& context, const ChannelPtr& ch, const SID& sid, const char* format, ...);

#ifndef LOGME_DISABLE_STD_FORMAT
    template<typename... Args>
    void Log(const Context& context, const StdFormat*, Override& ovr, const char* fmt, Args&&... args)
    {
      std::string out = std::vformat(fmt, std::make_format_args(args...));
      Log(context, ovr, "%s", out.c_str());
    }
#endif
    LOGMELNK void Log(const Context& context, Override& ovr, const char* format, ...);

#ifndef LOGME_DISABLE_STD_FORMAT
    template<typename... Args>
    void Log(const Context& context, const StdFormat*, Override& ovr, const ID& id, const char* fmt, Args&&... args)
    {
      if (ShutdownCalled)
        return;

      std::string out = std::vformat(fmt, std::make_format_args(args...));
      Log(context, ovr, id, "%s", out.c_str());
    }

    template<typename... Args>
    void Log(const Context& context, const StdFormat*, Override& ovr, const ChannelPtr& ch, const char* fmt, Args&&... args)
    {
      if (ShutdownCalled)
        return;

      if (ch && context.ErrorLevel < Level::LEVEL_ERROR && ch->IsOutputActive(context) == false)
        return;

      std::string out = std::vformat(fmt, std::make_format_args(args...));
      Log(context, ovr, ch, "%s", out.c_str());
    }

    template<typename... Args>
    void Log(const Context& context, const StdFormat*, const ID& id, Override& ovr, const char* fmt, Args&&... args)
    {
      if (ShutdownCalled)
        return;

      std::string out = std::vformat(fmt, std::make_format_args(args...));
      Log(context, id, ovr, "%s", out.c_str());
    }

    template<typename... Args>
    void Log(const Context& context, const StdFormat*, const ChannelPtr& ch, Override& ovr, const char* fmt, Args&&... args)
    {
      if (ShutdownCalled)
        return;

      if (ch && context.ErrorLevel < Level::LEVEL_ERROR && ch->IsOutputActive(context) == false)
        return;

      std::string out = std::vformat(fmt, std::make_format_args(args...));
      Log(context, ch, ovr, "%s", out.c_str());
    }
#endif
    LOGMELNK void Log(const Context& context, Override& ovr, const ID& id, const char* format, ...);
    LOGMELNK void Log(const Context& context, Override& ovr, const ChannelPtr& ch, const char* format, ...);
    LOGMELNK void Log(const Context& context, const ID& id, Override& ovr, const char* format, ...);
    LOGMELNK void Log(const Context& context, Override& ovr, const ID& id, const SID& sid, const char* format, ...);
    LOGMELNK void Log(const Context& context, Override& ovr, const ChannelPtr& ch, const SID& sid, const char* format, ...);
    LOGMELNK void Log(const Context& context, const ChannelPtr& ch, Override& ovr, const char* format, ...);

#ifndef LOGME_DISABLE_STD_FORMAT
    template<typename... Args>
    void Log(const Context& context, const StdFormat*, const char* fmt, Args&&... args)
    {
      if (ShutdownCalled)
        return;

      std::string out = std::vformat(fmt, std::make_format_args(args...));
      Log(context, "%s", out.c_str());
    }
#endif
    LOGMELNK void Log(const Context& context, const char* format, ...);

    /// <summary>
    /// Returns existing channel or creates one when it does not exist.
    /// </summary>
    /// <param name="id">Channel id. Empty id selects default channel.</param>
    /// <returns>Channel pointer, or nullptr after shutdown.</returns>
    LOGMELNK ChannelPtr GetChannel(const ID& id);

    /// <summary>
    /// Returns channel only if it already exists.
    /// </summary>
    /// <param name="id">Channel id. Empty id selects default channel.</param>
    /// <returns>Existing channel pointer, or nullptr when named channel is not found.</returns>
    LOGMELNK ChannelPtr GetExistingChannel(const ID& id);

    /// <summary>
    /// Creates named channel or returns existing channel with same id.
    /// </summary>
    /// <param name="id">Channel id used as channel name.</param>
    /// <param name="flags">Initial output flags for new channel.</param>
    /// <param name="level">Initial minimum level accepted by new channel.</param>
    /// <returns>Channel pointer, or nullptr after shutdown.</returns>
    LOGMELNK ChannelPtr CreateChannel(
      const ID& id
      , const OutputFlags& flags = OutputFlags()
      , Level level = DEFAULT_LEVEL
    );

    /// <summary>
    /// Creates channel with automatically generated name.
    /// </summary>
    /// <param name="flags">Initial output flags for new channel.</param>
    /// <param name="level">Initial minimum level accepted by new channel.</param>
    /// <returns>Channel pointer, or nullptr after shutdown.</returns>
    LOGMELNK ChannelPtr CreateChannel(
      const OutputFlags& flags = OutputFlags()
      , Level level = DEFAULT_LEVEL
    );

    /// <summary>
    /// Deletes named channel immediately and removes its backends and link.
    /// </summary>
    /// <param name="id">Channel id to delete.</param>
    LOGMELNK void DeleteChannel(const ID& id);

    /// <summary>
    /// Schedules channel deletion after its backends become idle.
    /// </summary>
    /// <param name="id">Channel id to delete later.</param>
    LOGMELNK void Autodelete(const ID& id);

    /// <summary>
    /// Ensures default channel exists and optionally removes all current channels first.
    /// </summary>
    /// <param name="delete_all">true to delete all channels before creating default layout.</param>
    LOGMELNK void CreateDefaultChannelLayout(bool delete_all = true);

    /// <summary>
    /// Sets home directory used by file-based backends and file manager factory.
    /// </summary>
    /// <param name="path">Directory path. Non-empty path is normalized to end with directory separator.</param>
    LOGMELNK void SetHomeDirectory(const std::string& path);

    /// <summary>
    /// Returns current home directory.
    /// </summary>
    /// <returns>Directory path used by file-based backends.</returns>
    LOGMELNK const std::string& GetHomeDirectory() const;

    /// <summary>
    /// Returns file manager factory used by file-based backends.
    /// </summary>
    /// <returns>Mutable file manager factory.</returns>
    LOGMELNK FileManagerFactory& GetFileManagerFactory();

    /// <summary>
    /// Returns channel name used to duplicate error-level records.
    /// </summary>
    /// <returns>Shared error channel name, or nullptr when error channel is not set.</returns>
    LOGMELNK StringPtr GetErrorChannel();

    /// <summary>
    /// Sets channel name used to duplicate error-level records.
    /// </summary>
    /// <param name="name">Name of existing channel that should also receive error records.</param>
    LOGMELNK void SetErrorChannel(const std::string& name);

    /// <summary>
    /// Sets channel name used to duplicate error-level records.
    /// </summary>
    /// <param name="name">Name of existing channel that should also receive error records.</param>
    LOGMELNK void SetErrorChannel(const char* name);

    /// <summary>
    /// Sets channel used to duplicate error-level records.
    /// </summary>
    /// <param name="ch">Channel id of existing channel that should also receive error records.</param>
    LOGMELNK void SetErrorChannel(const ID& ch);

    /// <summary>
    /// Removes all channels, their links and backends.
    /// </summary>
    LOGMELNK void DeleteAllChannels();

    /// <summary>
    /// Starts runtime control server.
    /// </summary>
    /// <param name="c">Control server configuration.</param>
    /// <returns>true if server was started.</returns>
    LOGMELNK bool StartControlServer(const ControlConfig& c);

    /// <summary>
    /// Stops runtime control server and waits for active control threads to finish.
    /// </summary>
    LOGMELNK void StopControlServer();

    /// <summary>
    /// Sets SSL certificate and private key for control server.
    /// </summary>
    /// <param name="cert">X509 certificate object. Ownership is not transferred.</param>
    /// <param name="key">Private key object. Ownership is not transferred.</param>
    /// <returns>Operation result.</returns>
    LOGMELNK Result SetControlCertificate(
      X509* cert
      , EVP_PKEY* key
    );

    /// <summary>
    /// Executes runtime control command and returns textual response.
    /// </summary>
    /// <param name="command">Control command text.</param>
    /// <returns>Command response text.</returns>
    LOGMELNK std::string Control(const std::string& command);

    /// <summary>
    /// Sets custom handler for runtime control commands not handled by logger.
    /// </summary>
    /// <param name="handler">Handler that receives command text and writes response text.</param>
    LOGMELNK void SetControlExtension(TControlHandler handler);

    /// <summary>
    /// Sets global predicate checked by logging macros before writing records.
    /// </summary>
    /// <param name="cond">Condition callback, or nullptr to restore default always-true condition.</param>
    LOGMELNK void SetCondition(TCondition cond);

    /// <summary>
    /// Default logging condition.
    /// </summary>
    /// <returns>Always true.</returns>
    LOGMELNK static bool DefaultCondition();

    /// <summary>
    /// Calls callback for default channel and all named channels.
    /// </summary>
    /// <param name="callback">Function called with channel name and channel pointer.</param>
    LOGMELNK void IterateChannels(const TChannelCallback& callback);

    /// <summary>
    /// Checks whether file is currently used by file manager factory.
    /// </summary>
    /// <param name="file">Path to file to test.</param>
    /// <returns>true if file is in use by logger file manager.</returns>
    LOGMELNK bool TestFileInUse(const std::string& file);

    /// <summary>
    /// Enables or disables virtual terminal mode support.
    /// </summary>
    /// <param name="enable">true to enable VT mode support.</param>
    LOGMELNK void SetEnableVTMode(bool enable);

    /// <summary>
    /// Returns virtual terminal mode support flag.
    /// </summary>
    /// <returns>true if VT mode support is enabled.</returns>
    LOGMELNK bool GetEnableVTMode() const;

    /// <summary>
    /// Sets or clears key used for deobfuscating records.
    /// </summary>
    /// <param name="key">Obfuscation key copied by logger, or nullptr to disable deobfuscation.</param>
    LOGMELNK void SetObfuscationKey(const ObfKey* key);

    /// <summary>
    /// Returns currently configured obfuscation key.
    /// </summary>
    /// <returns>Pointer to internal key, or nullptr when deobfuscation is disabled.</returns>
    LOGMELNK const ObfKey* GetObfuscationKey() const;

    /// <summary>
    /// Deobfuscates binary record to readable log line.
    /// </summary>
    /// <param name="r">Pointer to obfuscated record data.</param>
    /// <param name="len">Record size in bytes.</param>
    /// <param name="line">Output string receiving deobfuscated line.</param>
    /// <returns>true if record was decoded.</returns>
    LOGMELNK bool DeobfuscateRecord(const uint8_t* r, size_t len, std::string& line) const;

  protected:
    ChannelPtr CreateChannelInternal(
      const ID& id
      , const OutputFlags& flags
      , Level level = DEFAULT_LEVEL
    );

    void ApplyThreadChannel(Context& context);
    bool CreateChannels(ChannelConfigArray& arr);
    void ReplaceChannels(ChannelConfigArray& arr);

    void ControlListener();
    void ControlHandler(int socket);

    void DoAutodelete(bool force);
    void FreeControlSsl();

  public:
    /// <summary>
    /// Handles control command that lists available command groups.
    /// </summary>
    /// <param name="arr">Parsed command tokens.</param>
    /// <param name="response">Output response text.</param>
    /// <returns>true if command was handled.</returns>
    static bool CommandList(StringArray& arr, std::string& response);

    /// <summary>
    /// Handles control command for channel management.
    /// </summary>
    /// <param name="arr">Parsed command tokens.</param>
    /// <param name="response">Output response text.</param>
    /// <returns>true if command was handled.</returns>
    static bool CommandChannel(StringArray& arr, std::string& response);

    /// <summary>
    /// Handles control command for subsystem filter management.
    /// </summary>
    /// <param name="arr">Parsed command tokens.</param>
    /// <param name="response">Output response text.</param>
    /// <returns>true if command was handled.</returns>
    static bool CommandSubsystem(StringArray& arr, std::string& response);

    /// <summary>
    /// Handles control command for backend management.
    /// </summary>
    /// <param name="arr">Parsed command tokens.</param>
    /// <param name="response">Output response text.</param>
    /// <returns>true if command was handled.</returns>
    static bool CommandBackend(StringArray& arr, std::string& response);

    /// <summary>
    /// Handles control command for channel output flags.
    /// </summary>
    /// <param name="arr">Parsed command tokens.</param>
    /// <param name="response">Output response text.</param>
    /// <returns>true if command was handled.</returns>
    static bool CommandFlags(StringArray& arr, std::string& response);

    /// <summary>
    /// Handles control command for channel level filters.
    /// </summary>
    /// <param name="arr">Parsed command tokens.</param>
    /// <param name="response">Output response text.</param>
    /// <returns>true if command was handled.</returns>
    static bool CommandLevel(StringArray& arr, std::string& response);
  };

  typedef std::shared_ptr<Logger> LoggerPtr;
  LOGMELNK extern LoggerPtr Instance;

  inline ChannelPtr PCH()
  {
    return Instance ? Instance->GetDefaultChannelPtr() : nullptr;
  }

#ifndef LOGME_DISABLE_STD_FORMAT
  inline StdFormat* GetStdFormat()
  {
    return nullptr;
  }
#endif
}

