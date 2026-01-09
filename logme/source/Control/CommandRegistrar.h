#pragma once

#include "CommandDescriptor.h"

namespace Logme
{
  struct CommandRegistrar
  {
    CommandRegistrar(CommandDescriptor* d);
  };
}

#define COMMAND_DESCRIPTOR(c, h) \
  static bool h(Logme::StringArray& arr, std::string& response); \
  static Logme::CommandDescriptor Descriptor(c, h); \
  static Logme::CommandRegistrar Registrar(&Descriptor);

#define COMMAND_DESCRIPTOR2(c, h) \
  static Logme::CommandDescriptor Descriptor(c, h); \
  static Logme::CommandRegistrar Registrar(&Descriptor);
