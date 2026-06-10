#include <Logme/Logme.h>

#include <cstdlib>
#include <string>

static Logme::ID EnvExampleChannel{ "env_example" };

static void SetEnvironmentVariable(
  const std::string& name
  , const std::string& value
)
{
#ifdef _WIN32
  _putenv_s(name.c_str(), value.c_str());
#else
  setenv(name.c_str(), value.c_str(), 1);
#endif
}

static std::string GetEnvironmentVariable(const std::string& name)
{
#ifdef _WIN32
  char* value = nullptr;
  size_t size = 0;

  errno_t error = _dupenv_s(&value, &size, name.c_str());
  if (error != 0 || value == nullptr)
  {
    return std::string();
  }

  std::string result(value);
  std::free(value);
  return result;
#else
  const char* value = std::getenv(name.c_str());
  if (value == nullptr)
  {
    return std::string();
  }

  return std::string(value);
#endif
}

static void SetupDemoEnvironment()
{
  std::string value = GetEnvironmentVariable("LOGME_CONTROL");
  if (!value.empty())
  {
    return;
  }

  SetEnvironmentVariable(
    "LOGME_CONTROL"
    , "channel --enable env_example; level --channel env_example debug"
  );
}

static Logme::ChannelPtr CreateExampleChannel()
{
  auto defaultChannel = Logme::Instance->GetDefaultChannelPtr();
  defaultChannel->SetFilterLevel(Logme::LEVEL_DEBUG);

  auto channel = Logme::Instance->CreateChannel(EnvExampleChannel);
  channel->AddLink(::CH);
  channel->SetFilterLevel(Logme::LEVEL_INFO);
  channel->SetEnabled(false);
  return channel;
}

int main()
{
  CreateExampleChannel();

  LogmeI("Environment control example");
  LogmeI("env_example channel is disabled before ApplyEnvironmentControl()");

  LogmeD(EnvExampleChannel, "debug message before environment control");
  LogmeI(EnvExampleChannel, "info message before environment control");

  SetupDemoEnvironment();

  std::string control = GetEnvironmentVariable("LOGME_CONTROL");
  LogmeI("LOGME_CONTROL is set to: %s", control.c_str());

  Logme::EnvironmentControlOptions options;
  options.Prefix = "LOGME";
  options.Policy = Logme::ControlPolicy::Safe();
  options.ErrorMode = Logme::EnvironmentControlErrorMode::CONTINUE_ON_ERROR;

  Logme::Instance->ApplyEnvironmentControl(options);

  LogmeI("env_example channel is controlled by environment commands now");
  LogmeD(EnvExampleChannel, "debug message after environment control");
  LogmeI(EnvExampleChannel, "info message after environment control");

  return 0;
}
