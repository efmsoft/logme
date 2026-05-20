#include <Logme/Backend/WindowsEventLogBackend.h>
#include <Logme/Channel.h>
#include <Logme/Logger.h>
#include <Logme/WindowsEventLog/WindowsEventLogManagerFactory.h>

#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

using namespace Logme;

namespace
{
  static std::string GetBaseName(const char* path)
  {
    if (!path || !*path)
      return "logme";

    const char* slash = strrchr(path, '/');
    const char* backslash = strrchr(path, '\\');
    const char* name = path;

    if (slash && slash + 1 > name)
      name = slash + 1;

    if (backslash && backslash + 1 > name)
      name = backslash + 1;

    std::string result(name);
    size_t dot = result.find_last_of('.');
    if (dot != std::string::npos)
      result.resize(dot);

    if (result.empty())
      return "logme";

    return result;
  }

  static std::string GetDefaultSource()
  {
#ifdef _WIN32
    char path[MAX_PATH + 1];
    DWORD size = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (size > 0 && size <= MAX_PATH)
    {
      path[size] = 0;
      return GetBaseName(path);
    }
#else
    char path[4096];
    ssize_t size = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (size > 0)
    {
      path[size] = 0;
      return GetBaseName(path);
    }
#endif

    return "logme";
  }

#ifdef _WIN32
  static WORD GetEventLogType(Level level)
  {
    if (level >= LEVEL_ERROR)
      return EVENTLOG_ERROR_TYPE;

    if (level == LEVEL_WARN)
      return EVENTLOG_WARNING_TYPE;

    return EVENTLOG_INFORMATION_TYPE;
  }
#endif
}

WindowsEventLogBackend::WindowsEventLogBackend(ChannelPtr owner)
  : Backend(owner, TYPE_ID)
  , Source(GetDefaultSource())
  , EventId(1000)
  , Category(0)
  , EventHandle(nullptr)
  , Registered(false)
  , ShutdownFlag(false)
  , ShutdownCalled(owner == nullptr)
{
}

WindowsEventLogBackend::~WindowsEventLogBackend()
{
  Freeze();
  Close();
}

bool WindowsEventLogBackend::IsAsyncSupported() const
{
  return true;
}

BackendConfigPtr WindowsEventLogBackend::CreateConfig()
{
  return std::make_shared<WindowsEventLogBackendConfig>();
}

bool WindowsEventLogBackend::ApplyConfig(BackendConfigPtr c)
{
  if (!Backend::ApplyConfig(c))
    return false;

  auto config = std::static_pointer_cast<WindowsEventLogBackendConfig>(c);

  if (!config->Source.empty())
    Source = config->Source;

  EventId = config->EventId;
  Category = config->Category;
  return true;
}

std::string WindowsEventLogBackend::FormatDetails()
{
  std::string result = GetAsync() ? "ASYNC" : "SYNC";
  result += ", Source=";
  result += Source;
  result += ", EventId=";
  result += std::to_string(EventId);
  result += ", Category=";
  result += std::to_string(Category);
  return result;
}

void WindowsEventLogBackend::Display(Context& context)
{
  int nc;
  const char* buffer = context.Apply(Owner, Owner->GetFlags(), nc);

  if (!GetAsync() || ShutdownFlag.load(std::memory_order_relaxed))
  {
    bool flushed = false;

    if (!ShutdownFlag.load(std::memory_order_relaxed))
    {
      flushed = GetFactory().PushAndFlush(
        this
        , context.ErrorLevel
        , buffer
        , static_cast<size_t>(nc)
      );
    }

    if (!flushed)
    {
      WritePreparedRecord(
        context.ErrorLevel
        , EventId
        , Category
        , buffer
        , static_cast<size_t>(nc)
      );
    }

    return;
  }

  RegisterIfNeeded();

  if (!GetFactory().Push(this, context.ErrorLevel, buffer, static_cast<size_t>(nc)))
  {
    WritePreparedRecord(
      context.ErrorLevel
      , EventId
      , Category
      , buffer
      , static_cast<size_t>(nc)
    );
  }
}

void WindowsEventLogBackend::Flush()
{
  GetFactory().Flush();
}

void WindowsEventLogBackend::Freeze()
{
  if (Owner == nullptr)
    return;

  ShutdownFlag.store(true, std::memory_order_relaxed);
  Backend::Freeze();

  if (Registered.exchange(false, std::memory_order_relaxed))
    GetFactory().Remove(this);
  else
    GetFactory().Flush();

  Close();
}

bool WindowsEventLogBackend::IsIdle() const
{
  GetFactory().Flush();

  return ShutdownFlag.load(std::memory_order_relaxed) || Registered.load(std::memory_order_relaxed) == false;
}

const std::string& WindowsEventLogBackend::GetSource() const
{
  return Source;
}

uint32_t WindowsEventLogBackend::GetEventId() const
{
  return EventId;
}

uint16_t WindowsEventLogBackend::GetCategory() const
{
  return Category;
}

WindowsEventLogManagerFactory& WindowsEventLogBackend::GetFactory() const
{
  auto logger = Owner->GetOwner();
  return logger->GetWindowsEventLogManagerFactory();
}

void WindowsEventLogBackend::RegisterIfNeeded()
{
  if (Registered.load(std::memory_order_relaxed))
    return;

  WindowsEventLogBackendPtr self = std::static_pointer_cast<WindowsEventLogBackend>(shared_from_this());
  GetFactory().Add(self);
  Registered.store(true, std::memory_order_relaxed);
}

void WindowsEventLogBackend::OnShutdown()
{
  ShutdownCalled.store(true, std::memory_order_relaxed);
}

void WindowsEventLogBackend::Close()
{
#ifdef _WIN32
  if (EventHandle == nullptr)
    return;

  DeregisterEventSource(static_cast<HANDLE>(EventHandle));
  EventHandle = nullptr;
#endif
}

bool WindowsEventLogBackend::WritePreparedRecord(Level level, uint32_t eventId, uint16_t category, const char* text, size_t len)
{
#ifdef _WIN32
  (void)len;

  if (EventHandle == nullptr)
  {
    EventHandle = RegisterEventSourceA(
      nullptr
      , Source.c_str()
    );
  }

  if (EventHandle == nullptr)
    return false;

  const char* strings[] = { text };

  BOOL ok = ReportEventA(
    static_cast<HANDLE>(EventHandle)
    , GetEventLogType(level)
    , category
    , eventId
    , nullptr
    , 1
    , 0
    , strings
    , nullptr
  );

  return ok != FALSE;
#else
  (void)level;
  (void)eventId;
  (void)category;
  (void)text;
  (void)len;
  return true;
#endif
}
