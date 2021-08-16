#pragma once

#include "Backend/Backend.h"
#include "Context.h"
#include "OutputFlags.h"
#include "Types.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Logme
{
  class Channel
  {
    std::mutex DataLock;

    class Logger* Owner;
    std::string Name;
    OutputFlags Flags;
    Level LevelFilter;

    BackendArray Backends;
    uint64_t AccessCount;

  public:
    Channel(
      Logger* owner
      , const char* name
      , const OutputFlags& flags
      , Level level
    );

    bool operator==(const char* name) const;
    void Display(Context& context, const char* line);

    const OutputFlags& GetFlags() const;
    void SetFlags(const OutputFlags& flags);

    Level GetFilterLevel() const;
    void SetFilterLevel(Level level);

    void AddBackend(BackendPtr backend);

    Logger* GetOwner() const;
    std::string GetName();
    uint64_t GetAccessCount() const;
  };

  typedef std::shared_ptr<Channel> ChannelPtr;
  typedef std::vector<ChannelPtr> ChannelArray;
}
