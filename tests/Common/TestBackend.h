#pragma once

#include <Logme/Backend/Backend.h>
#include <Logme/Channel.h>

#include <string>
#include <vector>

struct TestBackend : public Logme::Backend
{
  std::string Line;
  std::vector<std::string> History;

  TestBackend(Logme::ChannelPtr owner)
    : Backend(owner, "TestBackend")
  {
  }

  void Clear()
  {
    Line.clear();
    History.clear();
  }

  void Display(Logme::Context& context) override
  {
    auto& flags = Owner->GetFlags();

    int nc{};
    Line = context.Apply(Owner, flags, nc);
    History.push_back(Line);
  }
};
