#pragma once

#include <memory>
#include <string>

#include <Logme/Types.h>

namespace Logme
{
  struct SID
  {
    uint64_t Name;

    LOGMELNK static SID Build(uint64_t name);
    LOGMELNK static SID Build(const char* name_upto8chars);
    LOGMELNK static SID Build(const std::string& name_upto8chars);
  };

  typedef std::shared_ptr<SID> SIDPtr;
}

static constexpr const Logme::SID SUBSID{0};

#define LOGME_SUBSYSTEM(var, name) \
  Logme::SID var = Logme::SID::Build(name)
