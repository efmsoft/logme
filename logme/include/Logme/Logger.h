#pragma once

#include <format>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <Logme/Channel.h>
#include <Logme/File/FileManagerFactory.h>
#include <Logme/Stream.h>
#include <Logme/Utils.h>

namespace Logme
{
  typedef std::shared_ptr<std::string> StringPtr;
  typedef std::function<bool(const std::string&, std::string&)> TControlHandler;
  typedef bool (*TCondition)();
  typedef std::function<void(const std::string&, const ChannelPtr&)> TChannelCallback;

  struct StdFormat{};

  class Logger : public std::enable_shared_from_this<Logger>
  {
    std::recursive_mutex DataLock;

    ChannelMap Channels;
    ChannelPtr Default;
    std::string HomeDirectory;

    int IDGenerator;

    std::map<uint64_t, ID> ThreadChannel;
    std::map<uint64_t, Override> ThreadOverride;
    FileManagerFactory Factory;

    std::mutex ErrorLock;
    StringPtr ErrorChannel;

    int ControlSocket;
    TControlHandler ControlExtension;
    
    typedef std::shared_ptr<std::thread> ThreadPtr;
    ThreadPtr ListenerThread;

    ChannelArray ToDelete;
    unsigned LastDoAutodelete;
    int NumDeleting;

    struct ControlThread
    {
      ThreadPtr Thread;
      bool Stopped;

      ControlThread() : Stopped(false)
      {
      }
    };

    typedef std::map<int, ControlThread> ThreadMap;
    ThreadMap ControlThreads;

  public:
    TCondition Condition;

  public:
    LOGMELNK Logger();
    LOGMELNK virtual ~Logger();

    LOGMELNK static void Shutdown();

    LOGMELNK void SetThreadChannel(const ID* id);
    LOGMELNK ID GetDefaultChannel();

    LOGMELNK void SetThreadOverride(const Override* ovr);
    LOGMELNK Override GetThreadOverride();

    LOGMELNK bool LoadConfigurationFile(const std::wstring& config_file, const std::string& section = std::string());
    LOGMELNK bool LoadConfigurationFile(const std::string& config_file, const std::string& section = std::string());
    LOGMELNK bool LoadConfiguration(const std::string& config_data, const std::string& section = std::string());

    LOGMELNK virtual void DoLog(Context& context, const char* format, va_list args);

    LOGMELNK Stream Log(const Context& context);

    LOGMELNK Stream Log(const Context& context, Override& ovr);
    LOGMELNK Stream Log(const Context& context, const ID& id);
    LOGMELNK Stream Log(const Context& context, ChannelPtr ch);
    LOGMELNK Stream Log(const Context& context, const ID& id, Override& ovr);
    LOGMELNK Stream Log(const Context& context, ChannelPtr ch, Override& ovr);

    template<typename... Args>
    void Log(const Context& context, const StdFormat*, const ID& id, const char* fmt, Args&&... args)
    {
      std::string out = std::vformat(fmt, std::make_format_args(args...));
      Log(context, id, "%s", out.c_str());
    }
    template<typename... Args>
    void Log(const Context& context, const StdFormat*, ChannelPtr ch, const char* fmt, Args&&... args)
    {
      std::string out = std::vformat(fmt, std::make_format_args(args...));
      Log(context, ch, "%s", out.c_str());
    }
    LOGMELNK void Log(const Context& context, const ID& id, const char* format, ...);
    LOGMELNK void Log(const Context& context, ChannelPtr ch, const char* format, ...);

    template<typename... Args>
    void Log(const Context& context, const StdFormat*, Override& ovr, const char* fmt, Args&&... args)
    {
      std::string out = std::vformat(fmt, std::make_format_args(args...));
      Log(context, ovr, "%s", out.c_str());
    }
    LOGMELNK void Log(const Context& context, Override& ovr, const char* format, ...);

    template<typename... Args>
    void Log(const Context& context, const StdFormat*, const ID& id, Override& ovr, const char* fmt, Args&&... args)
    {
      std::string out = std::vformat(fmt, std::make_format_args(args...));
      Log(context, id, ovr, "%s", out.c_str());
    }
    template<typename... Args>
    void Log(const Context& context, const StdFormat*, ChannelPtr ch, Override& ovr, const char* fmt, Args&&... args)
    {
      std::string out = std::vformat(fmt, std::make_format_args(args...));
      Log(context, ch, ovr, "%s", out.c_str());
    }
    LOGMELNK void Log(const Context& context, const ID& id, Override& ovr, const char* format, ...);
    LOGMELNK void Log(const Context& context, ChannelPtr ch, Override& ovr, const char* format, ...);

    template<typename... Args>
    void Log(const Context& context, const StdFormat*, const char* fmt, Args&&... args)
    {
      std::string out = std::vformat(fmt, std::make_format_args(args...));
      Log(context, "%s", out.c_str());
    }
    LOGMELNK void Log(const Context& context, const char* format, ...);

    LOGMELNK ChannelPtr GetChannel(const ID& id);
    LOGMELNK ChannelPtr GetExistingChannel(const ID& id);

    LOGMELNK ChannelPtr CreateChannel(
      const ID& id
      , const OutputFlags& flags = OutputFlags()
      , Level level = DEFAULT_LEVEL
    );
    LOGMELNK ChannelPtr CreateChannel(
      const OutputFlags& flags = OutputFlags()
      , Level level = DEFAULT_LEVEL
    );
    LOGMELNK void DeleteChannel(const ID& id);
    LOGMELNK void Autodelete(const ID& id);
    LOGMELNK void CreateDefaultChannelLayout(bool delete_all = true);

    LOGMELNK const std::string& GetHomeDirectory() const;
    LOGMELNK FileManagerFactory& GetFileManagerFactory();

    LOGMELNK StringPtr GetErrorChannel();
    LOGMELNK void SetErrorChannel(const std::string& name);
    LOGMELNK void SetErrorChannel(const char* name);
    LOGMELNK void SetErrorChannel(const ID& ch);

    LOGMELNK void DeleteAllChannels();
    LOGMELNK bool StartControlServer(const ControlConfig& c);
    LOGMELNK void StopControlServer();

    LOGMELNK std::string Control(const std::string& command);
    LOGMELNK void SetControlExtension(TControlHandler handler);

    LOGMELNK void SetCondition(TCondition cond);
    LOGMELNK static bool DefaultCondition();

    LOGMELNK void IterateChannels(const TChannelCallback& callback);

  protected:
    ChannelPtr CreateChannelInternal(
      const ID& id
      , const OutputFlags& flags
      , Level level = DEFAULT_LEVEL
    );

    void ApplyThreadChannel(Context& context);
    bool CreateChannels(ChannelConfigArray& arr);

    void ControlListener();
    void ControlHandler(int socket);

    void DoAutodelete(bool force);

  public:
    static bool CommandList(StringArray& arr, std::string& response);
  };

  typedef std::shared_ptr<Logger> LoggerPtr;
  LOGMELNK extern LoggerPtr Instance;

  inline StdFormat* GetStdFormat()
  {
    return nullptr;
  }
}

