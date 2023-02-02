#pragma once

#include <string>

#include <Logme/ID.h>

namespace Logme
{
  class SafeID : public ID
  {
    std::string Storage;

  public:
    SafeID();
    SafeID(const ID& id);
    SafeID(const SafeID& id);
    SafeID(const char* name);

    SafeID& operator=(const ID& id);
    SafeID& operator=(const SafeID& id);
    SafeID& operator=(const char* name);
  };
}
