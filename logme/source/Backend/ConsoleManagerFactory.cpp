#include <Logme/Backend/ConsoleManagerFactory.h>

using namespace Logme;

ConsoleManagerFactory::ConsoleManagerFactory()
{
}

ConsoleManagerFactory::~ConsoleManagerFactory()
{
  Instance.reset();
}

void ConsoleManagerFactory::Add(
  const ConsoleBackendPtr& backend
  , size_t maxRecords
  , size_t maxBytes
  , ConsoleOverflowPolicy policy
)
{
  std::unique_lock guard(Lock);

  if (Instance && Instance->Stopping())
  {
    std::shared_ptr<ConsoleManager> instance;
    Instance.swap(instance);
  }

  if (Instance == nullptr)
    Instance = std::make_shared<ConsoleManager>();

  Instance->AddBackend(backend, maxRecords, maxBytes, policy);
}

void ConsoleManagerFactory::Remove(ConsoleBackend* backend)
{
  std::unique_lock guard(Lock);
  std::shared_ptr<ConsoleManager> instance = Instance;
  guard.unlock();

  if (!instance)
    return;

  bool empty = instance->RemoveBackend(backend);
  if (!empty)
    return;

  guard.lock();
  if (Instance == instance)
  {
    std::shared_ptr<ConsoleManager> oldInstance;
    Instance.swap(oldInstance);
    guard.unlock();
  }
}

bool ConsoleManagerFactory::Push(
  ConsoleTarget target
  , Level level
  , bool highlight
  , const char* text
  , size_t len
)
{
  std::unique_lock guard(Lock);
  std::shared_ptr<ConsoleManager> instance = Instance;
  guard.unlock();

  if (!instance)
    return false;

  return instance->Push(target, level, highlight, text, len);
}

bool ConsoleManagerFactory::AppendRedirected(
  const ChannelPtr& owner
  , ConsoleTarget target
  , const char* text
  , size_t len
)
{
  std::unique_lock guard(Lock);

  if (Instance && Instance->Stopping())
  {
    std::shared_ptr<ConsoleManager> instance;
    Instance.swap(instance);
  }

  if (Instance == nullptr)
    Instance = std::make_shared<ConsoleManager>();

  std::shared_ptr<ConsoleManager> instance = Instance;
  guard.unlock();

  return instance->AppendRedirected(owner, target, text, len);
}

void ConsoleManagerFactory::Flush()
{
  std::unique_lock guard(Lock);
  std::shared_ptr<ConsoleManager> instance = Instance;
  guard.unlock();

  if (instance)
    instance->Flush();
}

void ConsoleManagerFactory::Notify(ConsoleBackend* backend)
{
  std::unique_lock guard(Lock);
  std::shared_ptr<ConsoleManager> instance = Instance;
  guard.unlock();

  if (instance)
    instance->Notify(backend);
}

void ConsoleManagerFactory::SetLimits(size_t maxRecords, size_t maxBytes)
{
  std::unique_lock guard(Lock);
  std::shared_ptr<ConsoleManager> instance = Instance;
  guard.unlock();

  if (instance)
    instance->SetLimits(maxRecords, maxBytes);
}

void ConsoleManagerFactory::SetOverflowPolicy(ConsoleOverflowPolicy policy)
{
  std::unique_lock guard(Lock);
  std::shared_ptr<ConsoleManager> instance = Instance;
  guard.unlock();

  if (instance)
    instance->SetOverflowPolicy(policy);
}

ConsoleQueueCounters ConsoleManagerFactory::GetCounters()
{
  std::unique_lock guard(Lock);
  std::shared_ptr<ConsoleManager> instance = Instance;
  guard.unlock();

  if (!instance)
    return ConsoleQueueCounters{};

  return instance->GetCounters();
}

void ConsoleManagerFactory::SetStopping()
{
  std::unique_lock guard(Lock);
  std::shared_ptr<ConsoleManager> instance = Instance;
  guard.unlock();

  if (instance)
    instance->SetStopping();
}
