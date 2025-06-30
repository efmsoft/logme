#pragma once

#include <Logme/Backend/Backend.h>
#include <Logme/File/file_io.h>
#include <Logme/Types.h>

namespace Logme
{
  struct SharedFileBackendConfig : public BackendConfig
  {
    size_t MaxSize;
    size_t Timeout;
    std::string Filename;

    LOGMELNK SharedFileBackendConfig();
    LOGMELNK ~SharedFileBackendConfig();

    LOGMELNK bool Parse(const Json::Value* po) override;
  };

  class SharedFileBackend
    : public Backend
    , public FileIo
  {
    size_t MaxSize;
    size_t Timeout;
    std::string Name;
    std::string NameTemplate;

  public:
    enum
    {
      MAX_SIZE_DEFAULT = 8 * 1024 * 1024,
    };

    constexpr static const char* TYPE_ID = "SharedFileBackend";

    LOGMELNK SharedFileBackend(ChannelPtr owner);
    LOGMELNK ~SharedFileBackend();

    LOGMELNK void SetMaxSize(size_t size);

    LOGMELNK void Display(Context& context, const char* line) override;
    LOGMELNK std::string GetPathName(int index = 0) override;

    LOGMELNK BackendConfigPtr CreateConfig() override;
    LOGMELNK bool ApplyConfig(BackendConfigPtr c) override;
    
    LOGMELNK std::string FormatDetails() override;

  private:
    bool CreateLog(const char* name);
    void CloseLog();

  };

  typedef std::shared_ptr<class SharedFileBackend> SharedFileBackendPtr;
}
