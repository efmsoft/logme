#pragma once

#include <Logme/OutputFlags.h>

namespace Logme
{
  struct Override
  {
    OutputFlags Add;
    OutputFlags Remove;

    Override();
  };
}