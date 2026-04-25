#pragma once

#include <string>

#include <Logme/ID.h>
#include <Logme/Types.h>

namespace Logme
{
  /// <summary>
  /// Owning version of Logme::ID. SafeID copies the channel name into internal storage and keeps
  /// ID::Name pointing to that stored string. Use it when a channel identifier must outlive the
  /// original const char* or temporary string used to create it.
  /// </summary>
  class SafeID : public ID
  {
    std::string Storage;

  public:
    /// <summary>Creates an empty channel identifier.</summary>
    LOGMELNK SafeID();
    /// <summary>Copies the name from a non-owning ID.</summary>
    LOGMELNK SafeID(const ID& id);
    /// <summary>Copies the name from another SafeID.</summary>
    LOGMELNK SafeID(const SafeID& id);
    /// <summary>Copies the supplied channel name. A null pointer creates an empty identifier.</summary>
    LOGMELNK SafeID(const char* name);

    /// <summary>Copies the name from a non-owning ID and updates ID::Name to the owned storage.</summary>
    LOGMELNK SafeID& operator=(const ID& id);
    /// <summary>Copies the name from another SafeID and updates ID::Name to the owned storage.</summary>
    LOGMELNK SafeID& operator=(const SafeID& id);
    /// <summary>Copies the supplied channel name and updates ID::Name to the owned storage.</summary>
    LOGMELNK SafeID& operator=(const char* name);
  };
  
  typedef std::shared_ptr<SafeID> IDPtr;
}
