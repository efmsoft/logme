#include <Logme/WindowsEventLog/WindowsEventLogManagerFactory.h>

using namespace Logme;

WindowsEventLogManagerFactory::WindowsEventLogManagerFactory()
{
}

WindowsEventLogManagerFactory::~WindowsEventLogManagerFactory()
{
  Instance.reset();
}

void WindowsEventLogManagerFactory::Add(const WindowsEventLogBackendPtr& backend)
{
  std::unique_lock guard(Lock);

  if (Instance && Instance->Stopping())
  {
    std::shared_ptr<WindowsEventLogManager> instance;
    Instance.swap(instance);
  }

  if (Instance == nullptr)
    Instance = std::make_shared<WindowsEventLogManager>();

  Instance->AddBackend(backend);
}

void WindowsEventLogManagerFactory::Remove(WindowsEventLogBackend* backend)
{
  std::unique_lock guard(Lock);
  std::shared_ptr<WindowsEventLogManager> instance = Instance;
  guard.unlock();

  if (!instance)
    return;

  bool empty = instance->RemoveBackend(backend);
  if (!empty)
    return;

  guard.lock();
  if (Instance == instance)
  {
    std::shared_ptr<WindowsEventLogManager> oldInstance;
    Instance.swap(oldInstance);
    guard.unlock();
  }
}

bool WindowsEventLogManagerFactory::Push(WindowsEventLogBackend* backend, Level level, const char* text, size_t len)
{
  std::unique_lock guard(Lock);
  std::shared_ptr<WindowsEventLogManager> instance = Instance;
  guard.unlock();

  if (!instance || instance->Stopping())
    return false;

  return instance->Push(backend, level, text, len);
}

bool WindowsEventLogManagerFactory::PushAndFlush(WindowsEventLogBackend* backend, Level level, const char* text, size_t len)
{
  std::unique_lock guard(Lock);
  std::shared_ptr<WindowsEventLogManager> instance = Instance;
  guard.unlock();

  if (!instance || instance->Stopping())
    return false;

  return instance->PushAndFlush(backend, level, text, len);
}

void WindowsEventLogManagerFactory::Flush()
{
  std::unique_lock guard(Lock);
  std::shared_ptr<WindowsEventLogManager> instance = Instance;
  guard.unlock();

  if (instance)
    instance->Flush();
}

void WindowsEventLogManagerFactory::SetStopping()
{
  std::unique_lock guard(Lock);
  std::shared_ptr<WindowsEventLogManager> instance = Instance;
  guard.unlock();

  if (instance)
    instance->SetStopping();
}

size_t WindowsEventLogManagerFactory::GetMemoryUsage()
{
  std::unique_lock guard(Lock);
  std::shared_ptr<WindowsEventLogManager> instance = Instance;
  guard.unlock();

  if (!instance)
    return 0;

  return instance->GetMemoryUsage();
}
