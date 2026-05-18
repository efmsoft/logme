#include <cassert>
#include <cerrno>
#include <cstring>

#include <Logme/Backend/FileBackend.h>
#include <Logme/File/buffered_file_io.h>
#include <Logme/Logme.h>

#ifndef _WIN32
#include <sys/file.h>
#include <unistd.h>
#define _close close
#define _fdopen fdopen
#define _fileno fileno
#define _lseek lseek
#define _read read
#else
#include <io.h>
#endif

#ifdef ENABLE_CLOSE_HOOK
#include <hook_api.h>
#endif

using namespace Logme;

#define BUFFERED_IO_ERROR(op) \
  {Error = errno; \
  std::string name = GetPathName(); \
  LogmeE(CHINT, "BufferedFileIo(%s): " #op " failed: %s", name.c_str(), ERRNO_STR(Error)); }

BufferedFileIo::BufferedFileIo(FileBackend* backend)
  : Backend(backend)
  , Stream(nullptr)
{
}

BufferedFileIo::~BufferedFileIo()
{
  Close();
}

std::string BufferedFileIo::GetPathName(int index)
{
  return Backend ? Backend->GetPathName(index) : std::string();
}

bool BufferedFileIo::Open(bool append, unsigned timeout, const char* fileName)
{
  Close();

  if (!FileIo::Open(append, timeout, fileName))
    return false;

#ifdef _WIN32
  Stream = _fdopen(File, "r+b");
#else
  Stream = fdopen(File, "r+b");
#endif
  if (!Stream)
  {
    BUFFERED_IO_ERROR(fdopen);
    FileIo::Close();
    return false;
  }

  return true;
}

void BufferedFileIo::Flush()
{
  std::lock_guard guard(IoLock);

  if (Stream && fflush(Stream) != 0)
    BUFFERED_IO_ERROR(fflush);
}

void BufferedFileIo::Close()
{
  std::lock_guard guard(IoLock);

  if (Stream)
  {
    FILE* fp = Stream;
    Stream = nullptr;

#if !defined(_WIN32) && !defined(__sun__)
    if (NeedUnlock)
    {
      int rc = flock(File, LOCK_UN);
      if (rc < 0)
        BUFFERED_IO_ERROR(flock);

      NeedUnlock = false;
    }
#endif

#ifdef ENABLE_CLOSE_HOOK
    CloseHookUnprotect(File);
#endif

    File = -1;

    int rc = fclose(fp);
    if (rc != 0)
      BUFFERED_IO_ERROR(fclose);
  }
  else
  {
    FileIo::Close();
  }
}

long long BufferedFileIo::Seek(size_t offs, int whence)
{
  std::lock_guard guard(IoLock);
  assert(Stream != nullptr);

  if (fflush(Stream) != 0)
  {
    BUFFERED_IO_ERROR(fflush);
    return -1;
  }

  long long rc = _lseek(File, (long)offs, whence);
  if (rc < 0)
    BUFFERED_IO_ERROR(lseek);

  return rc;
}

int BufferedFileIo::Truncate(size_t offs)
{
  std::lock_guard guard(IoLock);
  assert(Stream != nullptr);

  if (fflush(Stream) != 0)
  {
    BUFFERED_IO_ERROR(fflush);
    return -1;
  }

#ifdef _WIN32
  int rc = _chsize(File, (long)offs);
#else
  int rc = ftruncate(File, (long)offs);
#endif
  if (rc < 0)
    BUFFERED_IO_ERROR(ftruncate);

  return rc;
}

void BufferedFileIo::TruncateToMaxSize(size_t maxSize)
{
  if (Stream && fflush(Stream) != 0)
  {
    BUFFERED_IO_ERROR(fflush);
    return;
  }

  FileIo::TruncateToMaxSize(maxSize);
}

int BufferedFileIo::Write(const void* p, size_t size)
{
  std::lock_guard guard(IoLock);
  assert(Stream != nullptr);

  long long rc = Seek(0, SEEK_END);
  if (rc >= 0)
    return WriteRaw(p, size);

  return (int)rc;
}

int BufferedFileIo::WriteRaw(const void* p, size_t size)
{
  std::lock_guard guard(IoLock);
  assert(Stream != nullptr);

#if defined(LOGME_HAS_FWRITE_NOLOCK)
  size_t rc = _fwrite_nolock(p, 1, size, Stream);
#elif defined(LOGME_HAS_FWRITE_UNLOCKED)
  size_t rc = fwrite_unlocked(p, 1, size, Stream);
#else
  size_t rc = fwrite(p, 1, size, Stream);
#endif

  if (rc != size)
  {
    BUFFERED_IO_ERROR(fwrite);
    return -1;
  }

  return (int)rc;
}

int BufferedFileIo::Read(void* p, size_t size)
{
  std::lock_guard guard(IoLock);
  assert(Stream != nullptr);

  if (fflush(Stream) != 0)
  {
    BUFFERED_IO_ERROR(fflush);
    return -1;
  }

  int rc = _read(File, p, (unsigned)size);
  if (rc < 0)
    BUFFERED_IO_ERROR(read);

  return rc;
}

unsigned BufferedFileIo::Read(int maxLines, std::string& content, int part)
{
  return FileIo::Read(maxLines, content, part);
}
