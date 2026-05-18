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
#ifdef _WIN32
  static void WriteDebugText(const char* text)
  {
    OutputDebugStringA(text);
  }
#endif
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
  Freeze();
}

bool DebugBackend::IsAsyncSupported() const
{
  return true;
}

void DebugBackend::Flush()
{
  GetFactory().Flush();
}

void DebugBackend::Freeze()
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
  int nc;
  const char* buffer = context.Apply(Owner, Owner->GetFlags(), nc);

  if (!GetAsync() || ShutdownFlag.load(std::memory_order_relaxed))
  {
    bool flushed = false;

    if (!ShutdownFlag.load(std::memory_order_relaxed))
      flushed = GetFactory().PushAndFlush(buffer, static_cast<size_t>(nc));

    if (!flushed)
    {
#ifdef _WIN32
      WriteDebugText(buffer);
#else
      (void)buffer;
#endif
    }

    return;
  }

  RegisterIfNeeded();

  if (!GetFactory().Push(buffer, static_cast<size_t>(nc)))
  {
#ifdef _WIN32
    WriteDebugText(buffer);
#else
    (void)buffer;
#endif
  }
}

void DebugBackend::OnShutdown()
{
  ShutdownCalled.store(true, std::memory_order_relaxed);
}
