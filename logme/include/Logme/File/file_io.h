#pragma once

#include <mutex>
#include <stdio.h>
#include <string>

#include <Logme/CritSection.h>

#if !defined(_WIN32) && !defined(__sun__)
struct iovec;
#endif

namespace Logme
{
  class FileIo
  {
  public:
    FileIo();
    virtual ~FileIo();

    time_t GetLastWriteTime(int fd);

    virtual void Close();
    virtual void Flush();
    virtual bool Open(bool append, unsigned timeout = 0, const char* fileName = 0);
    virtual int Write(const void* p, size_t size);
    virtual int WriteRaw(const void* p, size_t size);
#if !defined(_WIN32) && !defined(__sun__)
    virtual int WriteRawVector(const struct iovec* iov, int iovcnt);
#endif
    virtual int Read(void* p, size_t size);
    virtual long long Seek(size_t offs, int whence);
    virtual int Truncate(size_t offs);
    virtual void TruncateToMaxSize(size_t maxSize);
    virtual unsigned Read(int maxLines, std::string& content, int part);
    bool IsOpen() const;

  protected:
    virtual std::string GetPathName(int index = 0) = 0;


  protected:
    CS IoLock;

    int File;
    int Error;
    bool NeedUnlock;
    time_t WriteTimeAtOpen;
  };
}
