#pragma once

#include <Logme/Backend/Backend.h>
#include <Logme/File/file_io.h>
#include <Logme/Types.h>

#include <condition_variable>
#include <mutex>

namespace Logme
{
  struct FileBackendConfig : public BackendConfig
  {
    bool Append;
    size_t MaxSize;
    std::string Filename;
    
    bool DailyRotation;
    int MaxParts;

    LOGMELNK FileBackendConfig();
    LOGMELNK ~FileBackendConfig();

    LOGMELNK bool Parse(const Json::Value* po) override;
  };

  class FileBackend 
    : public Backend
    , public FileIo
  {
    typedef std::vector<char> CharBuffer;
    typedef std::vector<size_t> SizeArray;

  private:
    bool Append;
    size_t MaxSize;
    size_t QueueSizeLimit;
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

    static size_t MaxSizeDefault;
    static size_t QueueSizeLimitDefault;
  
  public:
    enum 
    { 
      MAX_SIZE_DEFAULT = 8 * 1024 * 1024,
      
      FLUSH_PERIOD = 3000,                  // 3 sec
      QUEUE_SIZE_LIMIT = 8 * 1024 * 1024,   // force processing if queue size >= limit
      QUEUE_GROW_SIZE = 64 * 1024,          // grow size
      STAT_OUTPUT_PERIOD = 10 * 60 * 1000,  // 10 min
    };

    constexpr static const char* TYPE_ID = "FileBackend";

    LOGMELNK FileBackend(ChannelPtr owner);
    LOGMELNK ~FileBackend();

    LOGMELNK void SetAppend(bool append);
    LOGMELNK void SetMaxSize(size_t size);
    LOGMELNK void SetQueueLimit(size_t size);

    LOGMELNK bool CreateLog(const char* name);
    LOGMELNK void CloseLog();

    LOGMELNK void Display(Context& context, const char* line) override;
    LOGMELNK std::string GetPathName(int index = 0) override;

    LOGMELNK BackendConfigPtr CreateConfig() override;
    LOGMELNK bool ApplyConfig(BackendConfigPtr c) override;
    LOGMELNK bool IsIdle() const override;
    LOGMELNK void Freeze();

    LOGMELNK static size_t GetMaxSizeDefault();
    LOGMELNK static void SetMaxSizeDefault(size_t size);

    LOGMELNK static size_t GetQueueSizeLimitDefault();
    LOGMELNK static void SetQueueSizeLimitDefault(size_t size);

    LOGMELNK bool TestFileInUse(const std::string& file) const;

  private:
    class FileManagerFactory& GetFactory() const;

    void Truncate();
    void ConditionalFlush();

    void AppendString(const char* text, size_t len);
    void AppendOutputData(const char* text, size_t add);
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
