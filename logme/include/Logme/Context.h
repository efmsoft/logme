#pragma once

#include "ID.h"
#include "Module.h"
#include "OutputFlags.h"
#include "Types.h"

#include <memory>
#include <string>
#include <vector>

namespace Logme
{
  typedef std::shared_ptr<std::string> StringPtr;
  typedef const char* (*PfnAppend)(struct Context& context);

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
    }; 

    const ID* Channel;
    Level ErrorLevel;
    const char* Method;
    Module File;
    int Line;

    PfnAppend AppendProc;
    void* AppendContext;

    StringPtr Output;

    char Timestamp[TIMESTAMP_BUFFER_SIZE];
    char ThreadProcessID[TID_BUFFER_SIZE + PID_BUFFER_SIZE + 1];
    char Signature;

    OutputFlags Applied;
    const char* LastData;
    int LastLen;

    char Buffer[OUTPUT_BUFFER_SIZE];
    std::vector<char> ExtBuffer;

    struct Params
    {
      ID None;
      const ID& Channel;
      
      Params() 
        : None{0}
        , Channel(None)
      {
      }

      Params(const ID& ch)
        : None{0}
        , Channel(ch)
      {
      }
    };

  public:
    Context(Level level, const ID* ch);
    Context(Level level, const ID* chdef, const char* method, const char* module, int line, const Params& params);

    void InitContext();
    void InitTimestamp(TimeFormat tf);
    void InitSignature();
    void InitThreadProcessID(OutputFlags flags);
    void CreateTZD(char* tzd);

    const char* Apply(OutputFlags flags, const char* text, int& nc);
  };
}

#define LOGME_CONTEXT(level, ch, ...) \
  Logme::Context(level, ch, __FUNCTION__, __FILE__, __LINE__, Logme::Context::Params(__VA_ARGS__))
