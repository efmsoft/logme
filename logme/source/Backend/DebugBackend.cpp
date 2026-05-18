#include <Logme/Backend/DebugBackend.h>
#include <Logme/Channel.h>
#include <Logme/Debug/DebugManagerFactory.h>
#include <Logme/Logger.h>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace Logme;

namespace
{
  static void WriteDebugText(const char* text)
  {
#ifdef _WIN32
    OutputDebugStringA(text);
#else
    (void)text;
#endif
  }
}

DebugBackend::DebugBackend(ChannelPtr owner)
  : Backend(owner, TYPE_ID)
  , Registered(false)
  , ShutdownFlag(false)
  , ShutdownCalled(owner == nullptr)
{
}

DebugBackend::~DebugBackend()
{
  DoFreeze();
}

bool DebugBackend::IsAsyncSupported() const
{
  return true;
}

void DebugBackend::Flush()
{
  GetFactory().Flush();
}

void DebugBackend::DoFreeze()
{
  if (Owner == nullptr)
    return;

  ShutdownFlag.store(true, std::memory_order_relaxed);
  Backend::Freeze();

  if (Registered.exchange(false, std::memory_order_relaxed))
    GetFactory().Remove(this);
  else
    GetFactory().Flush();

}

void DebugBackend::Freeze()
{
  DoFreeze();
}

bool DebugBackend::IsIdle() const
{
  GetFactory().Flush();

  return ShutdownFlag.load(std::memory_order_relaxed) || Registered.load(std::memory_order_relaxed) == false;
}

std::string DebugBackend::FormatDetails()
{
  return GetAsync() ? "ASYNC" : "SYNC";
}

DebugManagerFactory& DebugBackend::GetFactory() const
{
  auto logger = Owner->GetOwner();
  return logger->GetDebugManagerFactory();
}

void DebugBackend::RegisterIfNeeded()
{
  if (Registered.load(std::memory_order_relaxed))
    return;

  DebugBackendPtr self = std::static_pointer_cast<DebugBackend>(shared_from_this());
  GetFactory().Add(self);
  Registered.store(true, std::memory_order_relaxed);
}

void DebugBackend::Display(Context& context)
{
#ifdef _WIN32
  int nc;
  const char* buffer = context.Apply(Owner, Owner->GetFlags(), nc);

  if (!GetAsync() || ShutdownFlag.load(std::memory_order_relaxed))
  {
    bool flushed = false;

    if (!ShutdownFlag.load(std::memory_order_relaxed))
      flushed = GetFactory().PushAndFlush(buffer, static_cast<size_t>(nc));

    if (!flushed)
      WriteDebugText(buffer);

    return;
  }

  RegisterIfNeeded();

  if (!GetFactory().Push(buffer, static_cast<size_t>(nc)))
    WriteDebugText(buffer);
#endif
}

void DebugBackend::OnShutdown()
{
  ShutdownCalled.store(true, std::memory_order_relaxed);
}
