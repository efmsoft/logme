#pragma once

#include <string>

#include <Logme/ID.h>
#include <Logme/Types.h>

namespace Logme
{
  class SafeID : public ID
  {
    std::string Storage;

  public:
    LOGMELNK SafeID();
    LOGMELNK SafeID(const ID& id);
    LOGMELNK SafeID(const SafeID& id);
    LOGMELNK SafeID(const char* name);

    LOGMELNK SafeID& operator=(const ID& id);
    LOGMELNK SafeID& operator=(const SafeID& id);
    LOGMELNK SafeID& operator=(const char* name);
  };
  
  typedef std::shared_ptr<SafeID> IDPtr;
}
