#include <algorithm>
#include <iterator>

#include <Logme/Backend/BufferBackend.h>
#include <Logme/Channel.h>

#include <cstring>

using namespace Logme;

BufferBackend::BufferBackend(Logme::ChannelPtr owner)
  : Backend(owner, TYPE_ID)
{
}

void BufferBackend::Clear()
{
  Buffer.clear();
}

void BufferBackend::Insert(const std::vector<char>& buffer)
{
  std::vector<char> data(buffer);
  std::copy(Buffer.begin(), Buffer.end(), std::back_inserter(data));
  std::swap(Buffer, data);
}

void BufferBackend::Display(Logme::Context& context, const char* line)
{
  OutputFlags flags = Owner->GetFlags();

  int nc;
  const char* str = context.Apply(flags, line, nc);

  size_t pos = Buffer.size();
  if (pos)
    pos--; // Overwrite last '\0'

  size_t c = Buffer.capacity();
  size_t s = pos + nc + 1;

  if (pos + nc + 1 < c)
  {
    c = ((s + GROW - 1) / GROW) * GROW;
    Buffer.reserve(c);
  }

  Buffer.resize(s);
  memcpy(&Buffer[pos], str, size_t(nc) + 1);
}

