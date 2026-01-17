#include <cassert>

#include <Logme/Backend/Backend.h>

#include <Logme/Backend/BufferBackend.h>
#include <Logme/Backend/ConsoleBackend.h>
#include <Logme/Backend/DebugBackend.h>
#include <Logme/Backend/FileBackend.h>
#include <Logme/Backend/SharedFileBackend.h>

#include <Logme/Logger.h>

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

void Backend::Flush()
{
}

BackendConfigPtr Backend::CreateConfig()
{
  return std::make_shared<BackendConfig>();
}

bool Backend::ApplyConfig(BackendConfigPtr c)
{
  (void)c;
  assert(c->Type == Type);
  return true;
}

std::string Backend::FormatDetails()
{
  return std::string();
}

BackendPtr Backend::Create(const char* type, ChannelPtr owner)
{
  if (ShutdownCalled)
    return BackendPtr();

  std::string t(type);

  if (t == BufferBackend::TYPE_ID)
    return std::make_shared<BufferBackend>(owner);

  if (t == ConsoleBackend::TYPE_ID)
    return std::make_shared<ConsoleBackend>(owner);

  if (t == DebugBackend::TYPE_ID)
    return std::make_shared<DebugBackend>(owner);

  if (t == FileBackend::TYPE_ID)
    return std::make_shared<FileBackend>(owner);

  if (t == SharedFileBackend::TYPE_ID)
    return std::make_shared<SharedFileBackend>(owner);

  return BackendPtr();
}