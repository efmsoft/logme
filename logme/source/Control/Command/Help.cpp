#include "../CommandRegistrar.h"

using namespace Logme;

COMMAND_DESCRIPTOR("help", CommandHelp);

static bool CommandHelp(Logme::StringArray& arr, std::string& response)
{
  response +=
    "list                       List channels\n"
    "help                       Print this help text\n"
  ;

  SortLines(response);
  return true;
}

