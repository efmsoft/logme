#include <cassert>
#include <chrono>
#include <filesystem>
#include <regex>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <vector>

#include <Logme/Backend/FileBackend.h>
#include <Logme/Channel.h>
#include <Logme/Context.h>
#include <Logme/File/exe_path.h>
#include <Logme/File/FileManagerFactory.h>
#include <Logme/Logger.h>
#include <Logme/Logme.h>
#include <Logme/Template.h>
#include <Logme/Time/datetime.h> 
#include <Logme/Types.h>

#include "../File/CleanOldest.h"

using namespace std::chrono_literals;

#ifndef _WIN32
#include <sys/file.h>
#include <unistd.h>

#define _lseek lseek
#define _read read
#define _write write

#define LOGME_SPRINTF_S(buf, bufsz, fmt, ...) \
  snprintf((buf), (bufsz), (fmt), __VA_ARGS__)
#else
#include <io.h>
#define ftruncate _chsize

#define LOGME_SPRINTF_S(buf, bufsz, fmt, ...) \
  sprintf_s((buf), (bufsz), (fmt), __VA_ARGS__)
#endif

using namespace Logme;

size_t FileBackend::MaxSizeDefault = FileBackend::MAX_SIZE_DEFAULT;
size_t FileBackend::QueueSizeLimitDefault = FileBackend::QUEUE_SIZE_LIMIT;

FileBackend::FileBackend(ChannelPtr owner)
  : Backend(owner, TYPE_ID)
  , Append(true)
  , MaxSize(MaxSizeDefault)
  , QueueSizeLimit(QueueSizeLimitDefault)
  , Registered(false)
  , CallScheduled(false)
  , DataReady(false)
  , ShutdownFlag(false)
  , ShutdownCalled(owner == nullptr)
  , FlushTime(0)
  , MaxBufferSize(0)
  , DailyRotation(false)
  , MaxParts(2)
{
  OutBuffer.reserve(QUEUE_GROW_SIZE);
  NonceGenInit(&Nonce);
}

FileBackend::~FileBackend()
{
  assert(ShutdownFlag || Registered == false);

  if (ShutdownCalled == false)
    WaitForShutdown();

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

size_t FileBackend::GetQueueSizeLimitDefault()
{
  return QueueSizeLimitDefault;
}

void FileBackend::SetQueueSizeLimitDefault(size_t size)
{
  if (size < 4ULL * 1024)
    size = 4ULL * 1024;

  QueueSizeLimitDefault = size;
}

std::string FileBackend::FormatDetails()
{
  std::string out = NameTemplate;
  if (Append)
    out += " APPEND";

  return out;
}

BackendConfigPtr FileBackend::CreateConfig()
{
  return std::make_shared<FileBackendConfig>();
}

void FileBackend::Flush()
{
  RequestFlush();

  std::unique_lock locker(BufferLock);
  Done.wait(locker, [this]() {return DataReady == false;});
}

void FileBackend::Freeze()
{
  ShutdownFlag = true;
  Backend::Freeze();

  RequestFlush();
}

bool FileBackend::IsIdle() const
{
  return DataReady == false && (ShutdownFlag || Registered == false);
}

uint64_t FileBackend::GetFlushTime() const
{
  return FlushTime;
}

bool FileBackend::ApplyConfig(BackendConfigPtr c)
{
  FileBackendConfig* p = (FileBackendConfig*)c.get();

  SetAppend(p->DailyRotation ? true : p->Append);
  SetMaxSize(p->MaxSize);

  DailyRotation = p->DailyRotation;
  MaxParts = p->MaxParts;
  
  Day.UpdateDayBoundaries();

  NameTemplate = p->Filename;
  return ChangePart();
}

void FileBackend::WaitForShutdown()
{
  while (ShutdownCalled == false && Registered)
  {
    std::unique_lock locker(BufferLock);

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

void FileBackend::SetQueueLimit(size_t size)
{
  assert(size > 1024);
  QueueSizeLimit = size;
}

void FileBackend::SetAppend(bool append)
{
  Append = append;
}

bool FileBackend::CreateLog(const char* v)
{
  NameTemplate = v;

  ProcessTemplateParam param;
  std::string name = ProcessTemplate(v, param);

  if (!IsAbsolutePath(name))
    name = Owner->GetOwner()->GetHomeDirectory() + name;

  CloseLog();
  Name.clear();

  Name = name;
  
  auto dir = std::filesystem::path(Name).parent_path();

  if (std::filesystem::exists(dir) == false)
    std::filesystem::create_directories(dir);

  return Open(Append);
}

void FileBackend::CloseLog()
{
  Close();
}

bool FileBackend::TestFileInUse(const std::string& file) const
{
  if (File == -1)
    return false;

  return Name == file;
}

size_t FileBackend::GetSize()
{
  if (File == -1)
    return static_cast<size_t>(-1);

  return (size_t)Seek(0, SEEK_END);
}

void FileBackend::Write(CharBuffer& data, SizeArray& msgSize)
{
  (void)msgSize;
  std::lock_guard guard(IoLock);
  
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

bool FileBackend::ChangePart()
{
  if (!CreateLog(NameTemplate.c_str()))
  {
    LogmeE(CHINT, "failed to create a new log");
    return false;
  }

  ProcessTemplateParam param(static_cast<uint32_t>(TEMPLATE_ALL) & ~static_cast<uint32_t>(TEMPLATE_DATE_AND_TIME));
  std::string re = ProcessTemplate(NameTemplate.c_str(), param);

  re = ReplaceDatetimePlaceholders(re, ".+");
  CleanFiles(std::regex(re), Name, MaxParts);

  return true;
}

void FileBackend::Display(Context& context, const char* line)
{
  if (File == -1 || ShutdownFlag)
    return;

  if (Registered == false)
  {
    std::unique_lock locker(BufferLock);

    if (Owner && Registered == false)
    {
      FileBackendPtr self = std::static_pointer_cast<FileBackend>(shared_from_this());
      GetFactory().Add(self);
      Registered = true;
    }
  }

  if (DailyRotation && Day.IsSameDayCached() == false)
  {
    if (!ChangePart())
      return;
  }

  int nc;
  const char* buffer = context.Apply(Owner, Owner->GetFlags(), line, nc);
  AppendString(buffer, nc);
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
    LOGME_SPRINTF_S(buffer, sizeof(buffer), "--- dropped %u characters\n", readPos + n);
    if (FileIo::WriteRaw(buffer, (int)strlen(buffer)) < 0)
      return;

    if (FileIo::WriteRaw(p, size_t(readSize) - n) < 0)
      return;

    long long nbytes = Seek(0, SEEK_CUR);
    if (nbytes < 0)
      return;

    if (FileIo::Truncate((size_t)nbytes) == -1)
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

    AppendObfuscated(text, len);
  }
}

void FileBackend::AppendObfuscated(const char* text, size_t add)
{
  auto key = Owner->GetOwner()->GetObfuscationKey();
  if (key == nullptr)
  {
    AppendOutputData(text, add);
    return;
  }

  auto output = ObfCalcRecordSize(add);
  if (output < 4ULL * 1024)
  {
    size_t size = 0;
    uint8_t buffer[4ULL * 1024];
    if (ObfEncryptRecord(key, &Nonce, (const uint8_t*)text, add, buffer, sizeof(buffer), &size))
      AppendOutputData((const char*)buffer, size);
  }

  size_t size = 0;
  std::vector<uint8_t> buffer(output);
  if (ObfEncryptRecord(key, &Nonce, (const uint8_t*)text, add, buffer.data(), buffer.size(), &size))
    AppendOutputData((const char*)buffer.data(), size);
}

void FileBackend::AppendOutputData(const char* text, size_t add)
{
  size_t queued = 0;
  size_t pos = static_cast<size_t>(-1);

  if (true)
  {
    std::lock_guard guard(BufferLock);
    
    if (!ShutdownFlag)
    {
      pos = OutBuffer.size();
      queued = pos + add;

      MessageSize.push_back(add);
      if (OutBuffer.capacity() < queued)
      {
        size_t n = ((queued / QUEUE_GROW_SIZE) + 1) * QUEUE_GROW_SIZE;
        OutBuffer.reserve(n);
      }

      OutBuffer.resize(queued);
      memcpy(&OutBuffer[pos], text, add);

      if (MaxBufferSize < queued)
        MaxBufferSize = (uint32_t)queued;

      DataReady = true;
    }
  }

  if (queued >= QueueSizeLimit)
    RequestFlush();                         // Speed up data processing
  else if (pos == 0)
    RequestFlush(uint64_t(GetTimeInMillisec()) + FLUSH_AFTER);
}

void FileBackend::RequestFlush(uint64_t when)
{
  FlushTime = when;
  CallScheduled = true;

  if (Owner)
    GetFactory().Notify(this, FlushTime);
}

bool FileBackend::HasEvents() const
{
  return CallScheduled;
}

FileManagerFactory& FileBackend::GetFactory() const
{
  auto logger = Owner->GetOwner();
  return logger->GetFileManagerFactory();
}

void FileBackend::GetOutputData(CharBuffer& data, SizeArray& size)
{
  std::lock_guard guard(BufferLock);

  assert(data.size() == 0);
  assert(size.size() == 0);

  data.swap(OutBuffer);
  size.swap(MessageSize);

  DataReady = false;
  assert(!data.empty());

  Done.notify_all();
}

void FileBackend::WriteData()
{
  SizeArray size;
  CharBuffer data;
  GetOutputData(data, size);

  Write(data, size);

  // Try to reuse buffers
  std::lock_guard guard(BufferLock);

  if (OutBuffer.size() == 0)
  {
    if (data.capacity() < QueueSizeLimit)
    {
      data.resize(0);
      size.resize(0);

      data.swap(OutBuffer);
      size.swap(MessageSize);
    }
  }
}

bool FileBackend::WorkerFunc()
{
  // Used to limit number of WriteData calls before Done event set if
  // one or more threads call Log() very frequently
  const int maxWriteLoops = 5;
  CallScheduled = false;

  if (DataReady)
  {
    for (int i = 0; DataReady && i < maxWriteLoops; i++)
      WriteData();
  }

  return !ShutdownFlag;
}

void FileBackend::OnShutdown()
{
  if (DataReady)
    WriteData();

  ShutdownCalled = true;
  Shutdown.notify_all();
}
