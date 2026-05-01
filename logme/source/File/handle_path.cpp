#include <Logme/File/exe_path.h>

#include <vector>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <limits.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef __APPLE__
#include <fcntl.h>
#endif
#endif

std::string Logme::GetFilePathFromFd(int fd)
{
  if (fd < 0)
    return std::string();

#ifdef _WIN32
  intptr_t osHandle = _get_osfhandle(fd);
  if (osHandle == -1)
    return std::string();

  HANDLE handle = reinterpret_cast<HANDLE>(osHandle);
  if (handle == INVALID_HANDLE_VALUE || handle == nullptr)
    return std::string();

  DWORD type = GetFileType(handle);
  if (type != FILE_TYPE_DISK)
    return std::string();

  DWORD size = GetFinalPathNameByHandleA(handle, nullptr, 0, FILE_NAME_NORMALIZED);
  if (size == 0)
    return std::string();

  std::vector<char> buffer(size + 1);
  DWORD rc = GetFinalPathNameByHandleA(handle, buffer.data(), (DWORD)buffer.size(), FILE_NAME_NORMALIZED);
  if (rc == 0 || rc >= buffer.size())
    return std::string();

  std::string path(buffer.data(), rc);
  const char prefix[] = "\\\\?\\";
  if (path.compare(0, sizeof(prefix) - 1, prefix) == 0)
    path.erase(0, sizeof(prefix) - 1);

  return path;
#elif defined(__APPLE__)
  struct stat st;
  if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode))
    return std::string();

  char buffer[PATH_MAX];
  if (fcntl(fd, F_GETPATH, buffer) == -1)
    return std::string();

  return std::string(buffer);
#else
  struct stat st;
  if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode))
    return std::string();

  char linkName[64];
  snprintf(linkName, sizeof(linkName), "/proc/self/fd/%d", fd);

  std::vector<char> buffer(PATH_MAX + 1);
  ssize_t rc = readlink(linkName, buffer.data(), buffer.size() - 1);
  if (rc <= 0)
    return std::string();

  buffer[(size_t)rc] = '\0';
  return std::string(buffer.data());
#endif
}

std::string Logme::GetFilePathFromFile(FILE* file)
{
  if (file == nullptr)
    return std::string();

#ifdef _WIN32
  int fd = _fileno(file);
#else
  int fd = fileno(file);
#endif

  return GetFilePathFromFd(fd);
}
