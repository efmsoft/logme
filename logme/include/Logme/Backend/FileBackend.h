#pragma once

#include <Logme/Backend/Backend.h>
#include <Logme/File/file_io.h>
#include <Logme/Types.h>

#include <mutex>

namespace Logme
{
  class FileBackend 
    : public Backend
    , public FileIo
  {
    typedef std::vector<char> CharBuffer;
    typedef std::vector<size_t> SizeArray;

  private:
    bool Append;
    size_t MaxSize;
    std::string Name;

    volatile bool DataReady;
    volatile long Flush;
    volatile bool ShutdownFlag;
    volatile bool ShutdownCalled;
    unsigned LastFlush;

    std::mutex BufferLock;
    CharBuffer OutBuffer;
    SizeArray MessageSize;
    uint32_t MaxBufferSize;

    std::condition_variable Done;
    std::condition_variable Shutdown;
  
  public:
    enum 
    { 
      MAX_SIZE_DEFAULT = 8 * 1024 * 1024 ,
      
      FLUSH_PERIOD = 3000,                  // 3 sec
      QUEUE_SIZE_LIMIT = 8 * 1024 * 1024,   // force processing if queue size >= limit
      QUEUE_GROW_SIZE = 64 * 1024,          // grow size
      STAT_OUTPUT_PERIOD = 10 * 60 * 1000,  // 10 min
    };

    FileBackend(ChannelPtr owner);
    ~FileBackend();

    void SetAppend(bool append);
    void SetMaxSize(size_t size);

    bool CreateLog(const char* name);
    void CloseLog();

    void Display(Context& context, const char* line) override;
    std::string GetPathName(int index = 0) override;

  private:
    void Truncate();
    void ConditionalFlush();

    void AppendString(const char* text, size_t len);
    void AppendOutputData(const CharBuffer& append);
    void RequestFlush();
    void GetOutputData(CharBuffer& data, SizeArray& size);
    void WriteData();

    void WaitForShutdown();

    void Write(CharBuffer&, SizeArray&);
    void FlushData();

    friend class FileManager;
    bool WorkerFunc(bool force);
    void OnShutdown();
  };
}
