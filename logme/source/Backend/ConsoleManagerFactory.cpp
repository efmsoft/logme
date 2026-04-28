#include <Logme/Backend/ConsoleManagerFactory.h>

using namespace Logme;

ConsoleManagerFactory::ConsoleManagerFactory()
{
}

ConsoleManagerFactory::~ConsoleManagerFactory()
{
  Instance.reset();
}

void ConsoleManagerFactory::Add(const ConsoleBackendPtr& backend)
{
  std::unique_lock guard(Lock);

  if (Instance && Instance->Stopping())
  {
    std::shared_ptr<ConsoleManager> instance;
    Instance.swap(instance);
  }

  if (Instance == nullptr)
    Instance = std::make_shared<ConsoleManager>();

  Instance->AddBackend(backend);
}

void ConsoleManagerFactory::Notify(ConsoleBackend* backend)
{
  std::unique_lock guard(Lock);
  std::shared_ptr<ConsoleManager> instance = Instance;
  guard.unlock();

  if (instance)
    instance->Notify(backend);
}

void ConsoleManagerFactory::SetStopping()
{
  std::unique_lock guard(Lock);
  std::shared_ptr<ConsoleManager> instance = Instance;
  guard.unlock();

  if (instance)
    instance->SetStopping();
}
