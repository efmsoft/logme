#pragma once

#include <mutex>
#include <string>
#include <stdio.h>

namespace Logme
{
  class FileIo
  {
  public:
    FileIo();
    ~FileIo();

    time_t GetLastWriteTime(int fd);

  protected:
    virtual std::string GetPathName(int index = 0) = 0;

    void Close();
    bool Open(bool append, unsigned timeout = 0, const char* fileName = 0);
    int Write(const void* p, size_t size);
    int WriteRaw(const void* p, size_t size);
    int Read(void* p, size_t size);
    long long Seek(size_t offs, int whence);
    int Truncate(size_t offs);

    unsigned Read(int maxLines, std::string& content, int part);

  protected:
    std::recursive_mutex IoLock;

    int File;
    int Error;
    bool NeedUnlock;
    time_t WriteTimeAtOpen;
  };
}
