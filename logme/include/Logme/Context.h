#pragma once

#include <Logme/ID.h>
#include <Logme/Module.h>
#include <Logme/OutputFlags.h>
#include <Logme/Override.h>
#include <Logme/Types.h>

#include <memory>
#include <string>
#include <vector>

namespace Logme
{
  typedef std::shared_ptr<std::string> StringPtr;
  typedef const char* (*PfnAppend)(struct Context& context);

  class Channel;
  typedef std::shared_ptr<Channel> ChannelPtr;

  struct ShortenerContext
  {
    std::string Buffer;
    char StaticBuffer[256];

    ShortenerContext()
      : StaticBuffer{0}
    {
    }
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

    ID ChannelStg;
    const ID* Channel;
    Level ErrorLevel;
    const char* Method;
    Module File;
    int Line;
    ChannelPtr Ch;

    PfnAppend AppendProc;
    void* AppendContext;

    Override Ovr;
    StringPtr Output;

    char Timestamp[TIMESTAMP_BUFFER_SIZE];
    char ThreadProcessID[TID_BUFFER_SIZE + PID_BUFFER_SIZE + 1];
    char Signature;

    OutputFlags Applied;
    const char* LastData;
    int LastLen;

    char Buffer[OUTPUT_BUFFER_SIZE];
    std::vector<char> ExtBuffer;

    ShortenerContext MethodShortener;

    struct Params
    {
      ID None;
      const ID& Channel;
      
      Params(...) 
        : None{0}
        , Channel(None)
      {
      }

      Params(const ID& ch, ...)
        : None{0}
        , Channel(ch)
      {
      }
    };

  public:
    LOGMELNK Context(Level level, const ID* ch);
    LOGMELNK Context(Level level, const ID* chdef, const char* method, const char* module, int line, const Params& params);

    LOGMELNK void InitContext();
    LOGMELNK void InitTimestamp(TimeFormat tf);
    LOGMELNK void InitSignature();
    LOGMELNK void InitThreadProcessID(ChannelPtr ch, OutputFlags flags);
    LOGMELNK void CreateTZD(char* tzd);

    LOGMELNK const char* Apply(ChannelPtr ch, OutputFlags flags, const char* text, int& nc);
  };
}

#define LOGME_CONTEXT(level, ch, ...) \
  Logme::Context(level, ch, __FUNCTION__, __FILE__, __LINE__, Logme::Context::Params(__VA_ARGS__))
