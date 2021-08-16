#pragma once

namespace Logme
{
  struct ID
  {
    const char* Name;
  };
}

static constexpr const Logme::ID CH{""};
static constexpr const Logme::ID CHINT{"logme"};

#define LOGME_CHANNEL(var, name) \
  Logme::ID var{name}
