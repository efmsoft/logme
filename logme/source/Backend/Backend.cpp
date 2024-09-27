#include <cassert>

#include <Logme/Backend/Backend.h>

#include <Logme/Backend/BufferBackend.h>
#include <Logme/Backend/ConsoleBackend.h>
#include <Logme/Backend/DebugBackend.h>
#include <Logme/Backend/FileBackend.h>

using namespace Logme;

Backend::Backend(ChannelPtr owner, const char* type)
  : Owner(owner)
  , Type(type)
  , Freezed(false)
{
}

Backend::~Backend()
{
}

const char* Backend::GetType() const
{
  return Type;
}

void Backend::Freeze()
{
  Freezed = true;
}

bool Backend::IsIdle() const
{
  return true;
}

BackendConfigPtr Backend::CreateConfig()
{
  return std::make_shared<BackendConfig>();
}

bool Backend::ApplyConfig(BackendConfigPtr c)
{
  assert(c->Type == Type);
  return true;
}

BackendPtr Backend::Create(const char* type, ChannelPtr owner)
{
  std::string t(type);

  if (t == BufferBackend::TYPE_ID)
    return std::make_shared<BufferBackend>(owner);

  if (t == ConsoleBackend::TYPE_ID)
    return std::make_shared<ConsoleBackend>(owner);

  if (t == DebugBackend::TYPE_ID)
    return std::make_shared<DebugBackend>(owner);

  if (t == FileBackend::TYPE_ID)
    return std::make_shared<FileBackend>(owner);

  return BackendPtr();
}