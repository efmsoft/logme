#pragma once

#include <cstddef>
#include <memory>

namespace Logme
{
  class DataBuffer
  {
    std::unique_ptr<char[]> DataPtr;
    std::size_t CapacityValue;
    std::size_t SizeValue;

  public:
    explicit DataBuffer(std::size_t capacity);

    DataBuffer(const DataBuffer&) = delete;
    DataBuffer& operator=(const DataBuffer&) = delete;

    std::size_t Capacity() const;
    std::size_t Size() const;

    const char* Data() const;
    char* Data();

    void Reset();

    bool CanAppend(std::size_t cb) const;
    void Append(const char* p, std::size_t cb);
  };

  typedef std::unique_ptr<DataBuffer> DataBufferPtr;
}
