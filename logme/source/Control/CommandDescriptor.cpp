#include "CommandDescriptor.h"

using namespace Logme;

CommandDescriptor* CommandDescriptor::Head = nullptr;

CommandDescriptor::CommandDescriptor(
  const char* command
  , CommandHandler handler
)
  : Next(nullptr)
  , Command(command)
  , Handler(handler)
{
}