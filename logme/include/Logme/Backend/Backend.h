#pragma once

#include <Logme/Types.h>

#include <memory>
#include <vector>

namespace Json
{
  class Value;
}

namespace Logme
{
  class Channel;
  struct Context;

  typedef std::shared_ptr<Channel> ChannelPtr;
  
  struct BackendConfig
  {
    std::string Type;

    LOGMELNK BackendConfig(const char* type);

    LOGMELNK virtual ~BackendConfig();

    /// <summary>
    /// Parses backend-specific configuration from JSON node.
    /// </summary>
    /// <param name="po">JSON object with backend configuration.</param>
    /// <returns>true if configuration was parsed.</returns>
    LOGMELNK virtual bool Parse(const Json::Value* po);
  };

  typedef std::shared_ptr<BackendConfig> BackendConfigPtr;
  typedef std::vector<BackendConfigPtr> BackendConfigArray;

  struct Backend;
  typedef std::shared_ptr<Backend> BackendPtr;
  typedef std::vector<BackendPtr> BackendArray;

  struct Backend : public std::enable_shared_from_this<Backend>
  {
    ChannelPtr Owner;
    const char* Type;
    bool Freezed;

  public:
    LOGMELNK Backend(ChannelPtr owner, const char* type);

    LOGMELNK virtual ~Backend();

    LOGMELNK virtual BackendConfigPtr CreateConfig();

    /// <summary>
    /// Applies backend-specific configuration.
    /// </summary>
    /// <param name="c">Configuration object matching this backend type.</param>
    /// <returns>true if configuration was applied.</returns>
    LOGMELNK virtual bool ApplyConfig(BackendConfigPtr c);

    /// <summary>
    /// Checks whether backend has no pending work.
    /// </summary>
    /// <returns>true if backend can be safely deleted by autodelete logic.</returns>
    LOGMELNK virtual bool IsIdle() const;

    /// <summary>
    /// Prevents backend from receiving new records.
    /// </summary>
    LOGMELNK virtual void Freeze();

    /// <summary>
    /// Flushes buffered backend output.
    /// </summary>
    LOGMELNK virtual void Flush();

    LOGMELNK const char* GetType() const;

    /// <summary>
    /// Creates built-in backend by type id.
    /// </summary>
    /// <param name="type">Backend type id.</param>
    /// <param name="owner">Channel that will own created backend.</param>
    /// <returns>Backend object, or nullptr when type is unknown or logger is shutting down.</returns>
    LOGMELNK static BackendPtr Create(const char* type, ChannelPtr owner);

    /// <summary>
    /// Returns backend-specific status details for diagnostics.
    /// </summary>
    /// <returns>Human-readable details string, or empty string when backend has no details.</returns>
    LOGMELNK virtual std::string FormatDetails();
  
  protected:
    friend class Channel;
    LOGMELNK virtual void Display(Context& context) = 0;
  };
}
