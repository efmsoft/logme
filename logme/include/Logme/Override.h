#pragma once

#include <Logme/OutputFlags.h>
#include <Logme/Types.h>

namespace Logme
{
  struct Override
  {
    OutputFlags Add;
    OutputFlags Remove;

    LOGMELNK Override();
  };
}