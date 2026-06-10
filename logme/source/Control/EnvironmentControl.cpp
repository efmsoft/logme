#include <cstdlib>
#include <string>
#include <vector>

#include <Logme/Logme.h>
#include <Logme/EnvironmentControl.h>
#include <Logme/Utils.h>

using namespace Logme;

namespace
{
  std::string GetProcessEnvironmentVariable(const std::string& name)
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
      return std::string();

    return std::string(value);
#endif
  }

  void SplitEnvironmentControlCommands(
    const std::string& value
    , std::vector<std::string>& commands
  )
  {
    size_t pos = 0;

    for (;;)
    {
      size_t next = value.find(';', pos);
      std::string command;

      if (next == std::string::npos)
        command = value.substr(pos);
      else
        command = value.substr(pos, next - pos);

      command = TrimSpaces(command);
      if (!command.empty())
        commands.push_back(command);

      if (next == std::string::npos)
        break;

      pos = next + 1;
    }
  }

  std::string GetControlCommandName(const std::string& command)
  {
    StringArray items;
    if (WordSplit(command, items) == 0)
      return "<empty>";

    ToLowerAsciiInplace(items[0]);
    return items[0];
  }

  bool IsControlErrorResponse(const std::string& response)
  {
    return response.rfind("error:", 0) == 0
      || response.rfind("{\"ok\":false", 0) == 0;
  }

  bool HandleEnvironmentControlError(
    const EnvironmentControlOptions& options
    , bool& success
    , const char* format
    , const std::string& value1 = std::string()
    , const std::string& value2 = std::string()
  )
  {
    if (value1.empty() && value2.empty())
    {
      LogmeE(CHINT, format);
    }
    else if (value2.empty())
    {
      LogmeE(CHINT, format, value1.c_str());
    }
    else
      LogmeE(CHINT, format, value1.c_str(), value2.c_str());

    if (options.ErrorMode != EnvironmentControlErrorMode::IGNORE_ERRORS)
      success = false;

    return options.ErrorMode != EnvironmentControlErrorMode::STOP_ON_ERROR;
  }

  bool ProcessEnvironmentControlVariable(
    Logger& logger
    , const EnvironmentControlOptions& options
    , const std::string& variable
    , const std::string& value
    , size_t& totalLength
    , size_t& commandCount
    , bool& success
  )
  {
    if (options.MaxVariableLength != 0 && value.size() > options.MaxVariableLength)
    {
      return HandleEnvironmentControlError(
        options
        , success
        , "environment control: variable is too long: %s"
        , variable
      );
    }

    if (options.MaxTotalLength != 0 && totalLength + value.size() > options.MaxTotalLength)
    {
      return HandleEnvironmentControlError(
        options
        , success
        , "environment control: total command length limit exceeded at variable: %s"
        , variable
      );
    }

    totalLength += value.size();

    std::vector<std::string> commands;
    SplitEnvironmentControlCommands(value, commands);

    for (size_t i = 0; i < commands.size(); ++i)
    {
      const std::string& command = commands[i];

      if (options.MaxCommands != 0 && commandCount >= options.MaxCommands)
      {
        (void)HandleEnvironmentControlError(
          options
          , success
          , "environment control: too many commands"
        );
        return false;
      }

      ++commandCount;

      if (options.MaxCommandLength != 0 && command.size() > options.MaxCommandLength)
      {
        if (!HandleEnvironmentControlError(
          options
          , success
          , "environment control: command is too long in variable: %s"
          , variable
        ))
          return false;

        continue;
      }

      std::string response = logger.Control(command, options.Policy);
      std::string commandName = GetControlCommandName(command);

      if (IsControlErrorResponse(response))
      {
        if (!HandleEnvironmentControlError(
          options
          , success
          , "environment control: command failed: %s: %s"
          , commandName
          , response
        ))
          return false;

        continue;
      }

      if (options.LogAppliedCommands)
        LogmeI(CHINT, "environment control: command applied: %s", commandName.c_str());
      else
        LogmeD(CHINT, "environment control: command applied: %s", commandName.c_str());
    }

    return true;
  }
}

EnvironmentControlOptions::EnvironmentControlOptions()
  : Prefix("LOGME")
  , Policy(ControlPolicy::Safe())
  , ErrorMode(EnvironmentControlErrorMode::CONTINUE_ON_ERROR)
  , MaxVariables(32)
  , MaxCommands(64)
  , MaxVariableLength(8192)
  , MaxCommandLength(2048)
  , MaxTotalLength(32768)
  , LogAppliedCommands(true)
{
}

bool Logger::ApplyEnvironmentControl(const EnvironmentControlOptions& options)
{
  bool success = true;

  if (options.Prefix.empty())
  {
    (void)HandleEnvironmentControlError(
      options
      , success
      , "environment control: prefix is empty"
    );
    return success;
  }

  size_t totalLength = 0;
  size_t commandCount = 0;

  std::string baseVariable = options.Prefix + "_CONTROL";
  std::string baseValue = GetProcessEnvironmentVariable(baseVariable);
  if (!baseValue.empty())
  {
    if (!ProcessEnvironmentControlVariable(
      *this
      , options
      , baseVariable
      , baseValue
      , totalLength
      , commandCount
      , success
    ))
      return success;
  }

  for (size_t i = 1; i <= options.MaxVariables; ++i)
  {
    std::string variable = options.Prefix + "_CONTROL_" + std::to_string(i);
    std::string value = GetProcessEnvironmentVariable(variable);
    if (value.empty())
      continue;

    if (!ProcessEnvironmentControlVariable(
      *this
      , options
      , variable
      , value
      , totalLength
      , commandCount
      , success
    ))
      return success;
  }

  return success;
}
