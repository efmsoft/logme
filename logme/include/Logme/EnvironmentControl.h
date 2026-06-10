#pragma once

#include <cstddef>
#include <string>

#include <Logme/ControlPolicy.h>
#include <Logme/Types.h>

namespace Logme
{

  enum class EnvironmentControlErrorMode
  {
    IGNORE_ERRORS,
    CONTINUE_ON_ERROR,
    STOP_ON_ERROR
  };

  struct EnvironmentControlOptions
  {
    std::string Prefix;
    ControlPolicy Policy;
    EnvironmentControlErrorMode ErrorMode;

    size_t MaxVariables;
    size_t MaxCommands;
    size_t MaxVariableLength;
    size_t MaxCommandLength;
    size_t MaxTotalLength;

    bool LogAppliedCommands;

    LOGMELNK EnvironmentControlOptions();
  };

}
