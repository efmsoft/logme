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
#endif

using namespace Logme;

FileIo::FileIo()
  : File(-1)
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
  if (File != -1)
  {
#if !defined(_WIN32) && !defined(__sun__)
    if (NeedUnlock)
    {
      int rc = flock(File, LOCK_UN);
      if (rc < 0)
        logmeE(CHINT, "FileIo(%s): flock(LOCK_UN) failed: %i", File, errno);

      NeedUnlock = false;
    }
#endif

    _close(File);
    File = -1;
  }
}

bool FileIo::Open(bool append, unsigned timeout, const char* fileName)
{
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
    int err = errno;
#endif
    if (File < 0)
    {
      unsigned t = GetTimeInMillisec();
      if (t < start || t - start >= timeout)
      {
        LogmeE(CHINT, "FileIo(%s): open failed. Error %i", fileName, err);
        return false;
      }
      continue;
    }

#ifndef _WIN32
    if (fcntl(File, F_SETFD, FD_CLOEXEC) == -1)
    {
      int err = errno;
      logmeE(CHINT, "FileIo(%s): flock(LOCK_EX) failed: %i", fileName, err);

      Close();
      return false;
    }

    assert(NeedUnlock == false);

#ifndef __sun__
    int rc = flock(File, LOCK_EX | LOCK_NB);
    if (rc < 0)
    {
      int err = errno;

      unsigned t = GetTimeInMillisec();
      if (t < start || t - start >= timeout)
      {
        logmeE(CHINT, "FileIo(%s): flock(LOCK_EX) failed: %i", fileName, err);

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
#ifdef _WIN32
  struct _stat st;
  int rc = _fstat(fd, &st);
#else
  struct stat st;
  int rc = fstat(fd, &st);
#endif

  if (rc < 0)
  {
    int e = errno;

    LogmeE(CHINT, "fstat() failed: %i", e);
    return time(0);
  }

  return st.st_mtime;
}

void FileIo::Write(const void* p, size_t size)
{
  assert(File != -1);

  _lseek(File, 0, SEEK_END);

  int rc = _write(File, p, (unsigned)size);
  if (rc < 0)
  {
    std::string pathName = GetPathName();
    LogmeE(CHINT, "FileIo(%s): write() failed: %i", pathName.c_str(), errno);
  }
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
