#pragma once

#include <string>
#include <Logme/Utils.h>

namespace Logme
{
  typedef bool (*CommandHandler)(Logme::StringArray& arr, std::string& response);

  struct CommandDescriptor
  {
    static CommandDescriptor* Head;
    CommandDescriptor* Next;

    const char* Command;
    CommandHandler Handler;

    CommandDescriptor(const char* command, CommandHandler handler);
  };
}