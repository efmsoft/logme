#pragma once

#include <stdint.h>
#include <string>

#include <Logme/Types.h>

namespace Logme
{
  enum TemplateFlags
  {
    TEMPLATE_PID = 0x00000001,
    TEMPLATE_PNAME = 0x00000002,
    TEMPLATE_PDATE = 0x00000004,
    TEMPLATE_TARGET = 0x00000008,
    TEMPLATE_EXEPATH = 0x00000010,
    TEMPLATE_PDATETIME = 0x00000020,

    TEMPLATE_ALL = 0xFFFFFFFF,
    TEMPLATE_ALL_CONST = TEMPLATE_PID | TEMPLATE_PNAME | TEMPLATE_EXEPATH,
    TEMPLATE_DATE_AND_TIME = TEMPLATE_PDATE | TEMPLATE_PDATETIME,
  };

  extern const char* TPID;
  extern const char* TPNAME;
  extern const char* TDATE;
  extern const char* TDATETIME;
  extern const char* TTARGET;
  extern const char* TEXEPATH;

  struct ProcessTemplateParam
  {
    ProcessTemplateParam(uint32_t flags = TEMPLATE_ALL)
      : Flags(flags)
      , TargetChannel(0)
    {
    }

    uint32_t Flags;
    const char* TargetChannel;
  }; 

  LOGMELNK std::string ProcessTemplate(
    const char* p
    , const ProcessTemplateParam& param
    , uint32_t* notProcessed = 0
  );

  LOGMELNK std::string ReplaceDatetimePlaceholders(
    const std::string& where
    , const std::string& on
  );

  LOGMELNK void EnvSetVar(const char* name, const char* value);
  LOGMELNK std::string EnvGetVar(const char* name);
}
