#include <Logme/Backend/CallbackBackend.h>
#include <Logme/Channel.h>
#include <Logme/Context.h>
#include <Logme/Logger.h>

using namespace Logme;

CallbackBackend::CallbackBackend(ChannelPtr owner)
  : Backend(owner, TYPE_ID)
  , Callback(nullptr)
  , UserData(nullptr)
{
}

CallbackBackend::CallbackBackend(
  ChannelPtr owner
  , CallbackBackendProc callback
  , void* userData
)
  : Backend(owner, TYPE_ID)
  , Callback(callback)
  , UserData(userData)
{
}

BackendConfigPtr CallbackBackend::CreateConfig()
{
  return std::make_shared<CallbackBackendConfig>();
}

bool CallbackBackend::ApplyConfig(BackendConfigPtr c)
{
  if (c == nullptr || c->Type != TYPE_ID)
    return false;

  return Backend::ApplyConfig(c);
}

std::string CallbackBackend::FormatDetails()
{
  if (Callback == nullptr)
    return "Callback=null";

  return "Callback=set";
}

void CallbackBackend::SetCallback(
  CallbackBackendProc callback
  , void* userData
)
{
  Callback = callback;
  UserData = userData;
}

CallbackBackendProc CallbackBackend::GetCallback() const
{
  return Callback;
}

void* CallbackBackend::GetUserData() const
{
  return UserData;
}

void CallbackBackend::Display(Context& context)
{
  if (Callback == nullptr)
    return;

  Callback(context, Owner, UserData);

  Logger* logger = Owner->GetOwner();
  if (logger->GetActiveLogStatisticsFast() != nullptr)
    logger->RecordLogBackendOutput(context, Owner.get(), this, 0);
}
