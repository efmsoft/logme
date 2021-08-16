#pragma once

#include "Backend.h"
#include "../File/file_io.h"
#include "../Types.h"

namespace Logme
{
  class FileBackend 
    : public Backend
    , public FileIo
  {
    bool Append;
    size_t MaxSize;
    std::string Name;
  
  public:
    enum { MAX_SIZE_DEFAULT = 8 * 1024 * 1024 };

    FileBackend(ChannelPtr owner);
    ~FileBackend();

    void SetAppend(bool append);
    void SetMaxSize(size_t size);

    bool CreateLog(const char* name);
    void CloseLog();

    void WriteLine(Context& context, const char* line);

    void Display(Context& context, const char* line) override;
    std::string GetPathName(int index = 0) override;

  private:
    void Truncate();
  };
}
