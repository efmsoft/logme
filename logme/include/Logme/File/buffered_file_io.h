#pragma once

#include <stdio.h>
#include <string>

#include <Logme/File/file_io.h>

namespace Logme
{
  class FileBackend;

  class BufferedFileIo : public FileIo
  {
  public:
    LOGMELNK BufferedFileIo(FileBackend* backend);
    LOGMELNK ~BufferedFileIo() override;

    LOGMELNK void Close() override;
    LOGMELNK void Flush() override;
    LOGMELNK bool Open(bool append, unsigned timeout = 0, const char* fileName = 0) override;
    LOGMELNK int Write(const void* p, size_t size) override;
    LOGMELNK int WriteRaw(const void* p, size_t size) override;
    LOGMELNK int Read(void* p, size_t size) override;
    LOGMELNK long long Seek(size_t offs, int whence) override;
    LOGMELNK int Truncate(size_t offs) override;
    LOGMELNK void TruncateToMaxSize(size_t maxSize) override;
    LOGMELNK unsigned Read(int maxLines, std::string& content, int part) override;

  protected:
    std::string GetPathName(int index = 0) override;

  private:
    FileBackend* Backend;
    FILE* Stream;
  };
}
