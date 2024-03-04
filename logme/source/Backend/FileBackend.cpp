#include <cassert>
#include <chrono>
#include <stdint.h>
#include <string.h>
#include <vector>

#include <Logme/Backend/FileBackend.h>
#include <Logme/Channel.h>
#include <Logme/Context.h>
#include <Logme/File/exe_path.h>
#include <Logme/File/FileManagerFactory.h>
#include <Logme/Logger.h>
#include <Logme/Template.h>
#include <Logme/Time/datetime.h> 
#include <Logme/Types.h>

using namespace std::chrono_literals;

#ifndef _WIN32
#include <sys/file.h>
#include <unistd.h>

#define _lseek lseek
#define _read read
#define _write write
#define sprintf_s sprintf
#else
#include <io.h>
#define ftruncate _chsize
#endif

using namespace Logme;

size_t FileBackend::MaxSizeDefault = FileBackend::MAX_SIZE_DEFAULT;

FileBackend::FileBackend(ChannelPtr owner)
  : Backend(owner, TYPE_ID)
  , Append(true)
  , MaxSize(MaxSizeDefault)
  , DataReady(false)
  , Flush(false)
  , ShutdownFlag(false)
  , ShutdownCalled(owner == nullptr)
  , LastFlush(0)
  , MaxBufferSize(0)
{
  if (Owner)
    GetFactory().Add(this);
}

FileBackend::~FileBackend()
{
  ShutdownFlag = true;
  RequestFlush();

  WaitForShutdown();

  if (Owner)
    GetFactory().Remove(this);

  CloseLog();
}

void FileBackend::SetMaxSizeDefault(size_t size)
{
  MaxSizeDefault = size;
}

size_t FileBackend::GetMaxSizeDefault()
{
  return MaxSizeDefault;
}

BackendConfigPtr FileBackend::CreateConfig()
{
  return std::make_shared<FileBackendConfig>();
}

bool FileBackend::ApplyConfig(BackendConfigPtr c)
{
  FileBackendConfig* p = (FileBackendConfig*)c.get();

  SetAppend(p->Append);
  SetMaxSize(p->MaxSize);

  return CreateLog(p->Filename.c_str());
}

void FileBackend::WaitForShutdown()
{
  while (ShutdownCalled == false)
  {
    std::unique_lock<std::mutex> locker(BufferLock);

    Shutdown.wait_for(
      locker
      , std::chrono::milliseconds(50)
      , [this]() {return ShutdownCalled; }
    );
  }
}

void FileBackend::SetMaxSize(size_t size)
{
  assert(size > 1024);
  MaxSize = size;
}

void FileBackend::SetAppend(bool append)
{
  Append = append;
}

bool FileBackend::CreateLog(const char* name)
{
  CloseLog();
  Name.clear();

  if (name)
    Name = name;

  return Open(Append);
}

void FileBackend::CloseLog()
{
  Close();
}

void FileBackend::Write(CharBuffer& data, SizeArray& msgSize)
{
  std::lock_guard<std::recursive_mutex> guard(IoLock);

  char* head = &data[0];
  char* tail = head + data.size();

  Truncate();

  while (head < tail)
  {
    long int cb = static_cast<long int>(tail - head);
    FileIo::Write(head, cb);
    break;
  }
}

void FileBackend::Display(Context& context, const char* line)
{
  int nc;
  const char* buffer = context.Apply(Owner->GetFlags(), line, nc);
  AppendString(buffer, nc);
}

void FileBackend::FlushData()
{
}

void FileBackend::Truncate()
{
  if (File == -1)
    return;

  auto rc = Seek(0, SEEK_END);
  if (rc == -1)
    return;

  if (rc < (long)MaxSize)
    return;

  uint32_t readPos = uint32_t(MaxSize / 2);
  uint32_t readSize = uint32_t(rc - readPos);

  std::vector<char> data(readSize);
  rc = Seek(readPos, SEEK_SET);
  if (rc == -1)
    return;

  if (Read(&data[0], readSize) < 0)
    return;

  const char* p = &data[0];
  while (*p && *p != '\n')
    p++;

  if (*p++)
  {
    uint32_t n = uint32_t(p - &data[0]);

    rc = Seek(0, SEEK_SET);
    if (rc < 0)
      return;

    char buffer[128];
    sprintf_s(buffer, "--- dropped %u characters\n", readPos + n);
    if (FileIo::WriteRaw(buffer, (int)strlen(buffer)) < 0)
      return;

    if (FileIo::WriteRaw(p, size_t(readSize) - n) < 0)
      return;

    long long nbytes = Seek(0, SEEK_CUR);
    if (nbytes < 0)
      return;

    if (FileIo::Truncate(nbytes) == -1)
      return;
  }

  rc = Seek(0, SEEK_END);
  if (rc < 0)
    return;
}

std::string FileBackend::GetPathName(int index)
{
  if (index < 0 || index > 1)
    return "";

  ProcessTemplateParam param;
  std::string name = ProcessTemplate(Name.c_str(), param);
  std::string pathName = name;

  if (!IsAbsolutePath(pathName))
    pathName = Owner->GetOwner()->GetHomeDirectory() + name;

  return pathName;   
}

void FileBackend::AppendString(const char* text, size_t len)
{
  if (text)
  {
    if (len == (size_t)-1)
      len = strlen(text);

    CharBuffer data;
    data.resize(len);
    memcpy(&data[0], text, len);

    AppendOutputData(data);
  }
}

void FileBackend::AppendOutputData(const CharBuffer& append)
{
  size_t queued = 0;

  if (true)
  {
    std::lock_guard<std::mutex> guard(BufferLock);
    
    if (!ShutdownFlag)
    {
      size_t add = append.size();
      size_t pos = OutBuffer.size();
      queued = pos + add;

      MessageSize.push_back(add);
      if (OutBuffer.capacity() < queued)
      {
        size_t n = ((queued / QUEUE_GROW_SIZE) + 1) * QUEUE_GROW_SIZE;
        OutBuffer.reserve(n);
      }

      OutBuffer.resize(queued);

      memcpy(&OutBuffer[pos], &append[0], add);
      if (MaxBufferSize < queued)
        MaxBufferSize = (uint32_t)queued;

      DataReady = true;
    }
  }

  if (queued >= QUEUE_SIZE_LIMIT)
    RequestFlush();                         // Speed up data processing
}

void FileBackend::RequestFlush()
{
  Flush = true;

  if (Owner)
    GetFactory().WakeUp();
}

FileManagerFactory& FileBackend::GetFactory() const
{
  auto logger = Owner->GetOwner();
  return logger->GetFileManagerFactory();
}

void FileBackend::GetOutputData(CharBuffer& data, SizeArray& size)
{
  std::lock_guard<std::mutex> guard(BufferLock);

  assert(data.size() == 0);
  assert(size.size() == 0);

  data.swap(OutBuffer);
  size.swap(MessageSize);

  DataReady = false;
  assert(!data.empty());
}

void FileBackend::WriteData()
{
  SizeArray size;
  CharBuffer data;
  GetOutputData(data, size);

  Write(data, size);

  // Try to reuse buffers
  std::lock_guard<std::mutex> guard(BufferLock);

  if (OutBuffer.size() == 0)
  {
    if (data.capacity() < QUEUE_SIZE_LIMIT)
    {
      data.resize(0);
      size.resize(0);

      data.swap(OutBuffer);
      size.swap(MessageSize);
    }
  }
}

void FileBackend::ConditionalFlush()
{
  unsigned ticks = GetTimeInMillisec();

  // Handle tick counter overflow
  if (ticks < LastFlush)
    LastFlush = ticks;

  if (Flush || ticks - LastFlush >= FLUSH_PERIOD)
  {
    FlushData();
    LastFlush = ticks;
  }
}

bool FileBackend::WorkerFunc(bool force)
{
  if (Flush || force)
  {
    // Used to limit number of WriteData calls before Done event set if
    // one or more threads call Log() very frequently
    const int maxWriteLoops = 5;

    if (DataReady)
    {
      for (int i = 0; DataReady && i < maxWriteLoops; i++)
        WriteData();

      ConditionalFlush();
      Done.notify_all();
    }
    else
    {
      ConditionalFlush();
      if (Flush)
        Done.notify_all();
    }

    Flush = false;
  }
  return !ShutdownFlag;
}

void FileBackend::OnShutdown()
{
  if (DataReady)
    WriteData();

  FlushData();

  ShutdownCalled = true;

  Done.notify_all();
  Shutdown.notify_all();
}
