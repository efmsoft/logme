#pragma once

#include <memory>

namespace Logme
{
  struct ID
  {
    const char* Name;
  };

  typedef std::shared_ptr<ID> IDPtr;
}

static constexpr const Logme::ID CH{""};
static constexpr const Logme::ID CHINT{"logme"};

#define LOGME_CHANNEL(var, name) \
  Logme::ID var{name}
