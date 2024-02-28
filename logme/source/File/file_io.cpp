#include <Logme/File/file_io.h>
#include <Logme/Logme.h>
#include <Logme/Time/datetime.h>

#include <cassert>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <vector>

#ifndef _WIN32
#include <sys/file.h>
#include <unistd.h>

#define _lseek lseek
#define _close close
#define _write write
#define _read read
#define _O_BINARY 0
#else
#include <io.h>
#define ftruncate _chsize
#endif

#ifdef ENABLE_CLOSE_HOOK
#include <hook_api.h>
#endif

using namespace Logme;

#define IO_ERROR(op) \
  {Error = errno; \
  std::string pathName = GetPathName(); \
  LogmeE(CHINT, "FileIo(%s): " #op " failed: %s", pathName.c_str(), ERRNO_STR(Error)); }

FileIo::FileIo()
  : File(-1)
  , Error(0)
  , NeedUnlock(false)
  , WriteTimeAtOpen(0)
{
}

FileIo::~FileIo()
{
  Close();
}

void FileIo::Close()
{
  std::lock_guard<std::recursive_mutex> guard(IoLock);

  if (File != -1)
  {
#if !defined(_WIN32) && !defined(__sun__)
    if (NeedUnlock)
    {
      int rc = flock(File, LOCK_UN);
      if (rc < 0)
        IO_ERROR(flock(LOCK_UN));

      NeedUnlock = false;
    }
#endif

    int h = File;
    File = -1;

#ifdef ENABLE_CLOSE_HOOK
    CloseHookUnprotect(h);
#endif

    int rc = _close(h);
    if (rc < 0)
      IO_ERROR(close());
  }
}

bool FileIo::Open(bool append, unsigned timeout, const char* fileName)
{
  std::lock_guard<std::recursive_mutex> guard(IoLock);
  assert(File == -1);

  std::string pathName;

  if (fileName == 0)
  {
    pathName = GetPathName();
    fileName = pathName.c_str();
  }

  int mode = O_RDWR | O_CREAT | _O_BINARY;
  if (!append)
    mode |= O_TRUNC;

  for (unsigned start = GetTimeInMillisec();; Sleep(1))
  {
#ifdef _WIN32
    errno_t err = _sopen_s(&File, fileName, mode | _O_NOINHERIT, _SH_DENYWR, _S_IREAD | _S_IWRITE);
#else
    const int pmode = S_IROTH | S_IWOTH | S_IWGRP | S_IRGRP | S_IWUSR | S_IRUSR;
    File = open(fileName, mode, pmode);
#endif
    Error = errno;

    if (File < 0)
    {
      unsigned t = GetTimeInMillisec();
      if (t < start || t - start >= timeout)
      {
        IO_ERROR(open);
        return false;
      }
      continue;
    }

#ifdef ENABLE_CLOSE_HOOK
    CloseHookProtect(File);
#endif

#ifndef _WIN32
    if (fcntl(File, F_SETFD, FD_CLOEXEC) == -1)
    {
      IO_ERROR(flock(LOCK_EX));

      Close();
      return false;
    }

    assert(NeedUnlock == false);

#ifndef __sun__
    int rc = flock(File, LOCK_EX | LOCK_NB);
    if (rc < 0)
    {
      Error = errno;

      unsigned t = GetTimeInMillisec();
      if (t < start || t - start >= timeout)
      {
        IO_ERROR(flock(LOCK_EX));

        Close();
        return false;
      }
      continue;
    }
    else
      NeedUnlock = true;
#endif
#endif
    break;
  }

  WriteTimeAtOpen = GetLastWriteTime(File);
  return true;
}

time_t FileIo::GetLastWriteTime(int fd)
{
  std::lock_guard<std::recursive_mutex> guard(IoLock);
  assert(File != -1);

#ifdef _WIN32
  struct _stat st;
  int rc = _fstat(fd, &st);
#else
  struct stat st;
  int rc = fstat(fd, &st);
#endif

  if (rc < 0)
  {
    IO_ERROR(fstat());
    return time(0);
  }

  return st.st_mtime;
}

long long FileIo::Seek(size_t offs, int whence)
{
  std::lock_guard<std::recursive_mutex> guard(IoLock);
  assert(File != -1);

  long long rc = _lseek(File, (long)offs, whence);
  if (rc < 0)
    IO_ERROR(lseek());

  return rc;
}

int FileIo::Truncate(size_t offs)
{
  std::lock_guard<std::recursive_mutex> guard(IoLock);
  assert(File != -1);

  int rc = ftruncate(File, (long)offs);
  if (rc < 0)
    IO_ERROR(ftruncate());

  return rc;
}

int FileIo::Write(const void* p, size_t size)
{
  std::lock_guard<std::recursive_mutex> guard(IoLock);
  assert(File != -1);

  long long rc = Seek(0, SEEK_END);
  if (rc >= 0)
  {
    return WriteRaw(p, size);
  }

  return (int) rc;
}

int FileIo::WriteRaw(const void* p, size_t size)
{
  std::lock_guard<std::recursive_mutex> guard(IoLock);
  assert(File != -1);

  int rc = _write(File, p, (unsigned int) size);
  if (rc < 0)
    IO_ERROR(write());

  return rc;
}

int FileIo::Read(void* p, size_t size)
{
  std::lock_guard<std::recursive_mutex> guard(IoLock);
  assert(File != -1);

  int rc = _read(File, p, (unsigned)size);
  if (rc < 0)
    IO_ERROR(read());

  return rc;
}

unsigned FileIo::Read(int maxLines, std::string& content, int part)
{
  content.clear();

  std::string pathName = GetPathName(part);

#ifdef _WIN32
  FILE* fp = nullptr;
  fopen_s(&fp, pathName.c_str(), "r");
#else
  FILE* fp = fopen(pathName.c_str(), "r");
#endif

  if (!fp)
    return RC_NO_ACCESS;

  std::string data;
  std::vector<size_t> offset;

  char buffer[4096];
  while (fgets(buffer, sizeof(buffer), fp))
  {
    offset.push_back(data.size());
    data += buffer;
  }

  if (!offset.empty())
  {
    size_t pos = 0;
    size_t n = offset.size();
    if (maxLines > 0 && (size_t)maxLines < n)
      pos = offset[n - 1 - maxLines];

    content = data.substr(pos);
  }

  fclose(fp);
  return RC_NOERROR;
}
