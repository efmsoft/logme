#include <Logme/Debug/DebugManagerFactory.h>

using namespace Logme;

DebugManagerFactory::DebugManagerFactory()
{
}

DebugManagerFactory::~DebugManagerFactory()
{
  Instance.reset();
}

void DebugManagerFactory::Add(const DebugBackendPtr& backend)
{
  std::unique_lock guard(Lock);

  if (Instance && Instance->Stopping())
  {
    std::shared_ptr<DebugManager> instance;
    Instance.swap(instance);
  }

  if (Instance == nullptr)
    Instance = std::make_shared<DebugManager>();

  Instance->AddBackend(backend);
}

void DebugManagerFactory::Remove(DebugBackend* backend)
{
  std::unique_lock guard(Lock);
  std::shared_ptr<DebugManager> instance = Instance;
  guard.unlock();

  if (!instance)
    return;

  bool empty = instance->RemoveBackend(backend);
  if (!empty)
    return;

  guard.lock();
  if (Instance == instance)
  {
    std::shared_ptr<DebugManager> oldInstance;
    Instance.swap(oldInstance);
    guard.unlock();
  }
}

bool DebugManagerFactory::Push(const char* text, size_t len)
{
  std::unique_lock guard(Lock);
  std::shared_ptr<DebugManager> instance = Instance;
  guard.unlock();

  if (!instance || instance->Stopping())
    return false;

  return instance->Push(text, len);
}

bool DebugManagerFactory::PushAndFlush(const char* text, size_t len)
{
  std::unique_lock guard(Lock);
  std::shared_ptr<DebugManager> instance = Instance;
  guard.unlock();

  if (!instance || instance->Stopping())
    return false;

  return instance->PushAndFlush(text, len);
}

void DebugManagerFactory::Flush()
{
  std::unique_lock guard(Lock);
  std::shared_ptr<DebugManager> instance = Instance;
  guard.unlock();

  if (instance)
    instance->Flush();
}

void DebugManagerFactory::SetStopping()
{
  std::unique_lock guard(Lock);
  std::shared_ptr<DebugManager> instance = Instance;
  guard.unlock();

  if (instance)
    instance->SetStopping();
}

size_t DebugManagerFactory::GetMemoryUsage()
{
  std::unique_lock guard(Lock);
  std::shared_ptr<DebugManager> instance = Instance;
  guard.unlock();

  if (!instance)
    return 0;

  return instance->GetMemoryUsage();
}
