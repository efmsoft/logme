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
  class Logger : public std::enable_shared_from_this<Logger>
  {
    std::mutex DataLock;

    ChannelArray Channels;
    ChannelPtr Default;
    std::string HomeDirectory;

    int IDGenerator;

    std::map<uint64_t, ID> ThreadChannel;
    FileManagerFactory Factory;

  public:
    Logger();
    virtual ~Logger();

    void SetThreadChannel(const ID* id);
    ID GetDefaultChannel();

    bool LoadConfigurationFile(const std::wstring& config_file, const std::string& section = std::string());
    bool LoadConfigurationFile(const std::string& config_file, const std::string& section = std::string());
    bool LoadConfiguration(const std::string& config_data, const std::string& section = std::string());

    virtual Stream DoLog(Context& context, const char* format, va_list args);

    Stream Log(const Context& context);

    Stream Log(const Context& context, const Override& ovr);
    Stream Log(const Context& context, const ID& id);
    Stream Log(const Context& context, const ID& id, const Override& ovr);

    Stream Log(const Context& context, const ID& id, const char* format, ...);
    Stream Log(const Context& context, const Override& ovr, const char* format, ...);
    Stream Log(const Context& context, const ID& id, const Override& ovr, const char* format, ...);

    Stream Log(const Context& context, const char* format, ...);

    ChannelPtr GetChannel(const ID& id);

    ChannelPtr CreateChannel(
      const ID& id
      , const OutputFlags& flags = OutputFlags()
      , Level level = DEFAULT_LEVEL
    );
    ChannelPtr CreateChannel(
      const OutputFlags& flags = OutputFlags()
      , Level level = DEFAULT_LEVEL
    );
    void DeleteChannel(const ID& id);
    void CreateDefaultChannelLayout(bool delete_all = true);

    const std::string& GetHomeDirectory() const;
    FileManagerFactory& GetFileManagerFactory();

  protected:
    ChannelPtr CreateChannelInternal(
      const ID& id
      , const OutputFlags& flags
      , Level level = DEFAULT_LEVEL
    );

    void ApplyThreadChannel(Context& context);
    void DeleteAllChannels();

    bool CreateChannels(ChannelConfigArray& arr);
  };

  typedef std::shared_ptr<Logger> LoggerPtr;
  extern LoggerPtr Instance;
}

