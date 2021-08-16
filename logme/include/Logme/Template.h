#pragma once

#include <stdint.h>
#include <string>

namespace Logme
{
  enum TemplateFlags
  {
    TEMPLATE_PID = 0x00000001,
    TEMPLATE_PNAME = 0x00000002,
    TEMPLATE_PDATE = 0x00000004,
    TEMPLATE_TARGET = 0x00000008,

    TEMPLATE_ALL = 0xFFFFFFFF,
    TEMPLATE_ALL_CONST = TEMPLATE_PID | TEMPLATE_PNAME,
  };

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

  std::string ProcessTemplate(const char* p, const ProcessTemplateParam& param, uint32_t* notProcessed = 0);

  void EnvSetVar(const char* name, const char* value);
  std::string EnvGetVar(const char* name); 
}
