#pragma once

#include <logme/OutputFlags.h>

namespace Logme
{
  struct Override
  {
    OutputFlags Add;
    OutputFlags Remove;

    Override();
  };
}