#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <regex>
#include <string>

#include <Logme/FastFormat.h>
#include <Logme/ID.h>
#include <Logme/Module.h>
#include <Logme/OutputFlags.h>
#include <Logme/Override.h>
#include <Logme/SID.h>
#include <Logme/Types.h>

namespace Logme
{
  typedef std::shared_ptr<std::string> StringPtr;
  typedef const char* (*PfnAppend)(struct Context& context);

  class Channel;
  typedef std::shared_ptr<Channel> ChannelPtr;

  enum class ContextCacheState : uint8_t
  {
    EMPTY,
    READY,
    DISABLED
  };

  struct ContextCache
  {
    std::atomic<ContextCacheState> State;
    FastFormatEntry Ffe;

    ContextCache()
      : State(ContextCacheState::EMPTY)
      , Ffe{}
    {
    }  
  };

  enum class CollapseMode : uint8_t
  {
    COUNT,
    TIME
  };

  struct CollapseEveryTag
  {
  };

  struct CollapseContextCache : public ContextCache
  {
    std::mutex Lock;
    std::string IgnoreRegexText;
    std::regex IgnoreRegex;
    std::string LastKey;
    CollapseMode Mode;
    uint64_t Limit;
    uint64_t IntervalMs;
    uint64_t LastOutputTime;
    uint64_t RepeatCount;
    bool IgnoreRegexEnabled;

    LOGMELNK CollapseContextCache(uint64_t limit);

    LOGMELNK CollapseContextCache(
      CollapseEveryTag tag
      , uint64_t intervalMs
    );

    LOGMELNK CollapseContextCache(const char* ignoreRegex, uint64_t limit);

    LOGMELNK CollapseContextCache(
      const char* ignoreRegex
      , CollapseEveryTag tag
      , uint64_t intervalMs
    );

    LOGMELNK void CompileIgnoreRegex();
  };

  struct ShortenerContext
  {
    std::unique_ptr<std::string> Buffer;
    char StaticBuffer[256];

    ShortenerContext()
    {
      StaticBuffer[0] = '\0';
    }

    ShortenerContext(const ShortenerContext&) = delete;
    ShortenerContext& operator=(const ShortenerContext&) = delete;

    ShortenerContext(ShortenerContext&&) noexcept = default;
    ShortenerContext& operator=(ShortenerContext&&) noexcept = default;
  };

  struct Context
  {
    enum
    {
      OUTPUT_BUFFER_SIZE = 2 * 1024,        // Size of preallocated buffer
      TIMESTAMP_BUFFER_SIZE = 32,
      TZD_BUFFER_SIZE = 16,
      TID_BUFFER_SIZE = 32,
      PID_BUFFER_SIZE = 32,
      TZD_T_POS = 10,                       // YYYY-MM-DDThh:mm:ssTZD
      TZD_E_POS = 19,
    };

    ContextCache& Cache;

    ID ChannelStg;
    const ID* Channel;
    SID Subsystem;
    Level ErrorLevel;
    const char* Method;
    Module File;
    int Line;
    
    ChannelPtr ChRef;
    Logme::Channel* Ch;

    PfnAppend AppendProc;
    void* AppendContext;

    Override* Ovr;
    StringPtr Output;
    CollapseContextCache* CollapseCache;
    uint64_t CollapseRepeatCount;

    char Timestamp[TIMESTAMP_BUFFER_SIZE];
    char ThreadProcessID[TID_BUFFER_SIZE + PID_BUFFER_SIZE + 1];
    char Signature;

    OutputFlags Applied;
    const char* LastData;
    int LastLen;
    bool LoggedBytesCounted;

    char Buffer[OUTPUT_BUFFER_SIZE];
    std::unique_ptr<char[]> ExtBuffer;
    size_t ExtBufferSize;

    const char* TempBuffer;
    size_t TempBufferSize;
    size_t TempBufferCapacity;
    std::string Storage;

    ShortenerContext MethodShortener;

    struct Params
    {
      ID None;
      const ID& Channel;

      SID SNone;
      const SID& Subsystem;

      LOGMELNK Params(...);
      LOGMELNK Params(const ID& ch, ...);
      LOGMELNK Params(const ChannelPtr& channel, ...);
      LOGMELNK Params(const SID& sid, ...);
      LOGMELNK Params(const ID& ch, const SID& sid, ...);
      LOGMELNK Params(const ChannelPtr& channel, const SID& sid, ...);
    };

  public:
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    Context(Context&&) noexcept = default;
    Context& operator=(Context&&) noexcept = delete;

    LOGMELNK Context(ContextCache& cache, Level level, const ID* ch, const SID* sid);
    LOGMELNK Context(ContextCache& cache, Level level, const ID* chdef, const SID* siddef, const char* method, const char* module, int line, const Params& params);

    LOGMELNK void InitContext();
    LOGMELNK void InitTimestamp(TimeFormat tf);
    LOGMELNK void InitSignature();
    LOGMELNK void InitThreadProcessID(const ChannelPtr& ch, OutputFlags flags);
    LOGMELNK void CreateTZD(char* tzd);
    LOGMELNK void SetText(const char* text);
    LOGMELNK void SetBuffer(const char* buffer, size_t size, size_t capacity);
    LOGMELNK bool ApplyCollapse();

    LOGMELNK const char* Apply(const ChannelPtr& ch, OutputFlags flags, int& nc);
    LOGMELNK const char* ApplyJson(const ChannelPtr& ch, OutputFlags flags, int& nc);
    LOGMELNK const char* ApplyXml(const ChannelPtr& ch, OutputFlags flags, int& nc);
  
    LOGMELNK const char* GetText() const;
  };
}

#define LOGME_CONTEXT(cache, level, ch, sid, ...) \
  Logme::Context(cache, level, ch, sid, __FUNCTION__, __FILE__, __LINE__, Logme::Context::Params(__VA_ARGS__))
