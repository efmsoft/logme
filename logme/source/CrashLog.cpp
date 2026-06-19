#include <cstdint>
#include <cstdio>
#include <cstdarg>

#include <Logme/Logger.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

using namespace Logme;

#if defined(_WIN32)
static const std::intptr_t INVALID_CRASH_HANDLE = reinterpret_cast<std::intptr_t>(INVALID_HANDLE_VALUE);
#else
static const std::intptr_t INVALID_CRASH_HANDLE = -1;
#endif

static bool IsValidCrashHandle(std::intptr_t value)
{
#if defined(_WIN32)
  return value != INVALID_CRASH_HANDLE && value != 0;
#else
  return value >= 0;
#endif
}

static void CloseCrashHandle(std::intptr_t value)
{
  if (!IsValidCrashHandle(value))
    return;

#if defined(_WIN32)
  CloseHandle(reinterpret_cast<HANDLE>(value));
#else
  close(static_cast<int>(value));
#endif
}

static void WriteCrashHandle(std::intptr_t value, const char* data, size_t size)
{
  if (!IsValidCrashHandle(value))
    return;

#if defined(_WIN32)
  HANDLE handle = reinterpret_cast<HANDLE>(value);
  while (size > 0)
  {
    DWORD chunk = size > static_cast<size_t>(0x7ffff000) ? 0x7ffff000 : static_cast<DWORD>(size);
    DWORD written = 0;
    if (!WriteFile(handle, data, chunk, &written, nullptr) || written == 0)
      break;
    data += written;
    size -= written;
  }
#else
  int fd = static_cast<int>(value);
  while (size > 0)
  {
    ssize_t written = write(fd, data, size);
    if (written <= 0)
      break;
    data += written;
    size -= static_cast<size_t>(written);
  }
#endif
}

bool Logger::OpenCrashLog(const char* file, bool append)
{
  if (!file || !*file)
    return false;

#if defined(_WIN32)
  DWORD disposition = append ? OPEN_ALWAYS : CREATE_ALWAYS;
  HANDLE handle = CreateFileA(
    file,
    FILE_APPEND_DATA,
    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    nullptr,
    disposition,
    FILE_ATTRIBUTE_NORMAL,
    nullptr
  );

  if (handle == INVALID_HANDLE_VALUE)
    return false;

  std::intptr_t value = reinterpret_cast<std::intptr_t>(handle);
#else
  int flags = O_WRONLY | O_CREAT;
  flags |= append ? O_APPEND : O_TRUNC;

  int fd = open(file, flags, 0666);
  if (fd < 0)
    return false;

  std::intptr_t value = fd;
#endif

  std::intptr_t old = CrashFileHandle.exchange(value);
  CloseCrashHandle(old);
  CrashOutputMask.fetch_or(CRASH_OUTPUT_FILE);
  return true;
}

void Logger::CloseCrashLog()
{
  std::intptr_t old = CrashFileHandle.exchange(INVALID_CRASH_HANDLE);
  CloseCrashHandle(old);
}

void Logger::SetCrashOutputMask(std::uint32_t mask)
{
  CrashOutputMask.store(mask);
}

std::uint32_t Logger::GetCrashOutputMask() const
{
  return CrashOutputMask.load();
}

void Logger::CrashWrite(const char* data, size_t size) noexcept
{
  CrashWrite(CrashOutputMask.load(), data, size);
}

void Logger::CrashWrite(std::uint32_t mask, const char* data, size_t size) noexcept
{
  if (!data || size == 0)
    return;

  if (mask & CRASH_OUTPUT_FILE)
    WriteCrashHandle(CrashFileHandle.load(), data, size);

#if defined(_WIN32)
  if (mask & CRASH_OUTPUT_STDERR)
    WriteCrashHandle(reinterpret_cast<std::intptr_t>(GetStdHandle(STD_ERROR_HANDLE)), data, size);
  if (mask & CRASH_OUTPUT_STDOUT)
    WriteCrashHandle(reinterpret_cast<std::intptr_t>(GetStdHandle(STD_OUTPUT_HANDLE)), data, size);
#else
  if (mask & CRASH_OUTPUT_STDERR)
    WriteCrashHandle(STDERR_FILENO, data, size);
  if (mask & CRASH_OUTPUT_STDOUT)
    WriteCrashHandle(STDOUT_FILENO, data, size);
#endif
}

void Logger::CrashLog(const char* format, ...) noexcept
{
  va_list args;
  va_start(args, format);
  std::uint32_t mask = CrashOutputMask.load();
  char buffer[2048];

  if (format)
  {
    int ret = vsnprintf(buffer, sizeof(buffer), format, args);
    if (ret > 0)
    {
      size_t len = static_cast<size_t>(ret);
      if (len >= sizeof(buffer))
        len = sizeof(buffer) - 1;

      CrashWrite(mask, buffer, len);
      if (buffer[len - 1] != '\n')
        CrashWrite(mask, "\n", 1);
    }
  }

  va_end(args);
}

void Logger::CrashLog(std::uint32_t mask, const char* format, ...) noexcept
{
  va_list args;
  va_start(args, format);
  char buffer[2048];

  if (format)
  {
    int ret = vsnprintf(buffer, sizeof(buffer), format, args);
    if (ret > 0)
    {
      size_t len = static_cast<size_t>(ret);
      if (len >= sizeof(buffer))
        len = sizeof(buffer) - 1;

      CrashWrite(mask, buffer, len);
      if (buffer[len - 1] != '\n')
        CrashWrite(mask, "\n", 1);
    }
  }

  va_end(args);
}
