#pragma once

#include <string>
#include <stdio.h>

namespace Logme
{
  class FileIo
  {
  public:
    FileIo();
    ~FileIo();

    static time_t GetLastWriteTime(int fd);

  protected:
    virtual std::string GetPathName(int index = 0) = 0;

    void Close();
    bool Open(bool append, unsigned timeout = 0, const char* fileName = 0);
    void Write(const void* p, size_t size);

    unsigned Read(int maxLines, std::string& content, int part);

  protected:
    int File;
    bool NeedUnlock;
    time_t WriteTimeAtOpen;
  };
}
