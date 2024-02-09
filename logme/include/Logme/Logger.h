#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <Logme/Channel.h>
#include <Logme/File/FileManagerFactory.h>
#include <Logme/Stream.h>

namespace Logme
{
  typedef std::shared_ptr<std::string> StringPtr;

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

  protected:
    ChannelPtr CreateChannelInternal(
      const ID& id
      , const OutputFlags& flags
      , Level level = DEFAULT_LEVEL
    );

    void ApplyThreadChannel(Context& context);

    bool CreateChannels(ChannelConfigArray& arr);
  };

  typedef std::shared_ptr<Logger> LoggerPtr;
  LOGMELNK extern LoggerPtr Instance;
}

