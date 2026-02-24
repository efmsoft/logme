#include <Logme/Buffer/DataBuffer.h>

#include <cstring>

using namespace Logme;

DataBuffer::DataBuffer(std::size_t capacity)
  : DataPtr(new char[capacity])
  , CapacityValue(capacity)
  , SizeValue(0)
{
}

std::size_t DataBuffer::Capacity() const
{
  return CapacityValue;
}

std::size_t DataBuffer::Size() const
{
  return SizeValue;
}

const char* DataBuffer::Data() const
{
  return DataPtr.get();
}

char* DataBuffer::Data()
{
  return DataPtr.get();
}

void DataBuffer::Reset()
{
  SizeValue = 0;
}

bool DataBuffer::CanAppend(std::size_t cb) const
{
  return cb <= (CapacityValue - SizeValue);
}

void DataBuffer::Append(const char* p, std::size_t cb)
{
  std::memcpy(DataPtr.get() + SizeValue, p, cb);
  SizeValue += cb;
}