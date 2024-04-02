#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <Logme/Channel.h>
#include <Logme/File/FileManagerFactory.h>
#include <Logme/Stream.h>

namespace Logme
{
  typedef std::shared_ptr<std::string> StringPtr;
  typedef std::function<bool(const std::string&, std::string&)> TControlHandler;
  typedef bool (*TCondition)();

  class Logger : public std::enable_shared_from_this<Logger>
  {
    std::mutex DataLock;

    ChannelMap Channels;
    ChannelPtr Default;
    std::string HomeDirectory;

    int IDGenerator;

    std::map<uint64_t, ID> ThreadChannel;
    FileManagerFactory Factory;

    std::mutex ErrorLock;
    StringPtr ErrorChannel;

    int ControlSocket;
    TControlHandler ControlExtension;
    
    typedef std::shared_ptr<std::thread> ThreadPtr;
    ThreadPtr ListenerThread;

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

    LOGMELNK void SetThreadChannel(const ID* id);
    LOGMELNK ID GetDefaultChannel();

    LOGMELNK bool LoadConfigurationFile(const std::wstring& config_file, const std::string& section = std::string());
    LOGMELNK bool LoadConfigurationFile(const std::string& config_file, const std::string& section = std::string());
    LOGMELNK bool LoadConfiguration(const std::string& config_data, const std::string& section = std::string());

    LOGMELNK virtual Stream DoLog(Context& context, const char* format, va_list args);

    LOGMELNK Stream Log(const Context& context);

    LOGMELNK Stream Log(const Context& context, const Override& ovr);
    LOGMELNK Stream Log(const Context& context, const ID& id);
    LOGMELNK Stream Log(const Context& context, const ID& id, const Override& ovr);

    LOGMELNK Stream Log(const Context& context, const ID& id, const char* format, ...);
    LOGMELNK Stream Log(const Context& context, const Override& ovr, const char* format, ...);
    LOGMELNK Stream Log(const Context& context, const ID& id, const Override& ovr, const char* format, ...);

    LOGMELNK Stream Log(const Context& context, const char* format, ...);

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
  };

  typedef std::shared_ptr<Logger> LoggerPtr;
  LOGMELNK extern LoggerPtr Instance;
}

