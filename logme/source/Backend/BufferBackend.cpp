#include <algorithm>
#include <cstring>
#include <iterator>

#include <Logme/Backend/BufferBackend.h>
#include <Logme/Channel.h>

using namespace Logme;

BufferBackend::BufferBackend(Logme::ChannelPtr owner)
  : Backend(owner, TYPE_ID)
{
}

BackendConfigPtr BufferBackend::CreateConfig()
{
  return std::make_shared<BufferBackendConfig>();
}

bool BufferBackend::ApplyConfig(BackendConfigPtr c)
{
  if (c == nullptr || c->Type != TYPE_ID)
    return false;

  BufferBackendConfig* p = (BufferBackendConfig*)c.get();

  std::lock_guard guard(Lock);
  Config = *p;

  return true;
}

void BufferBackend::Clear()
{
  std::lock_guard guard(Lock);
  Buffer.clear();
}

void BufferBackend::Append(BufferBackend& bb)
{
  std::vector<char> copy;

  if (true)
  {
    std::lock_guard guard(bb.Lock);

    if (bb.Buffer.empty())
      return;

    copy = bb.Buffer;
  }

  Append((const char*)copy.data(), -1);
} 

void BufferBackend::Append(const char* str, int nc)
{
  if (nc == -1)
    nc = (int)strlen(str);

  if (nc == 0)
    return;

  std::lock_guard guard(Lock);

  size_t pos = Buffer.size();
  if (pos)
    pos--; // Overwrite last '\0'

  if (Config.MaxSize != size_t(-1) && pos + nc + 1 > Config.MaxSize)
  {
    if (Config.Policy == BufferBackendPolicy::STOP_APPENDING)
      return;

    if (Config.Policy == BufferBackendPolicy::DELETE_OLDEST)
    {
      if (size_t(nc) + 1 > Config.MaxSize)
        return;

      do
      {
        void* p = memchr(Buffer.data(), '\n', Buffer.size());
        if (p)
        {
          size_t n = (char*)p - Buffer.data() + 1;
          Buffer.erase(Buffer.begin(), Buffer.begin() + n);
        }
        else
        {
          Buffer.clear();
        }

        pos = Buffer.size();
        if (pos)
          pos--;

      } while (pos + nc + 1 > Config.MaxSize);
    }
  }

  size_t c = Buffer.capacity();
  size_t s = pos + nc + 1;

  if (s > c)
  {
    c = ((s + GROW - 1) / GROW) * GROW;
    Buffer.reserve(c);
  }

  Buffer.resize(s);
  memcpy(&Buffer[pos], str, size_t(nc));
  Buffer[pos + nc] = '\0';
}

void BufferBackend::Display(Logme::Context& context, const char* line)
{
  OutputFlags flags = Owner->GetFlags();

  int nc;
  const char* str = context.Apply(Owner, flags, line, nc);

  Append(str, nc);
}

