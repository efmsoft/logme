#include "CommandRegistrar.h"

using namespace Logme;

CommandRegistrar::CommandRegistrar(CommandDescriptor* d)
{
  d->Next = CommandDescriptor::Head;
  CommandDescriptor::Head = d;
}