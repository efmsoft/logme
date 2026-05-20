#pragma once

#include <Logme/Backend/Backend.h>

namespace Logme
{
  typedef void (*CallbackBackendProc)(
    Context& context
    , const ChannelPtr& owner
    , void* userData
  );

  struct CallbackBackendConfig : public BackendConfig
  {
    LOGMELNK CallbackBackendConfig();
    LOGMELNK bool Parse(const Json::Value* po) override;
  };

  struct CallbackBackend : public Backend
  {
    constexpr static const char* TYPE_ID = "CallbackBackend";

  private:
    CallbackBackendProc Callback;
    void* UserData;

  public:
    LOGMELNK CallbackBackend(ChannelPtr owner);
    LOGMELNK CallbackBackend(
      ChannelPtr owner
      , CallbackBackendProc callback
      , void* userData = nullptr
    );

    LOGMELNK BackendConfigPtr CreateConfig() override;
    LOGMELNK bool ApplyConfig(BackendConfigPtr c) override;
    LOGMELNK std::string FormatDetails() override;

    LOGMELNK void SetCallback(
      CallbackBackendProc callback
      , void* userData = nullptr
    );

    LOGMELNK CallbackBackendProc GetCallback() const;
    LOGMELNK void* GetUserData() const;

    LOGMELNK void Display(Context& context) override;
  };

  typedef std::shared_ptr<CallbackBackend> CallbackBackendPtr;
}
