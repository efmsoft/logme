#include <algorithm>
#include <Logme/Utils.h>

#if defined(__GNUC__) && !defined(__DJGPP__)
#include <pthread.h> 
#if defined(__linux__)
#include <sys/syscall.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#elif defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif 

Logme::PRENAMETHREAD RenameThreadCallback;

void Logme::SetRenameThreadCallback(Logme::PRENAMETHREAD p)
{
  RenameThreadCallback = p;
}

void Logme::RenameThread(uint64_t threadID, const char* threadName)
{
  if (RenameThreadCallback)
    RenameThreadCallback(threadID, threadName);
}

std::string Logme::dword2str(uint32_t value)
{
  char buff[32];
  snprintf(buff, sizeof(buff), "%u", (unsigned)value);

  return std::string(buff);
} 

void Logme::SortLines(std::string& s)
{
  StringArray lines;
  WordSplit(s, lines, "\n");

  std::sort(lines.begin(), lines.end());

  s.clear();
  for (auto& l : lines)
    s += l + "\n";
}

size_t Logme::WordSplit(
  const std::string& s
  , StringArray& words
  , const char* delimiters
  , bool trim
  , bool ignoreEmpty
)
{
  size_t count{};
  size_t start{};
  size_t end{};

  words.clear();

  while (start < s.length())
  {
    end = s.find_first_of(delimiters, start);

    if (end == std::string::npos)
      end = s.length();

    auto token{ s.substr(start, end - start) };

    if (trim)
    {
      constexpr const char* spaces{ " \t\n\r\f\v" };
      token.erase(token.find_last_not_of(spaces) + 1);
      token.erase(0, token.find_first_not_of(spaces));
    }

    if (!ignoreEmpty || !token.empty())
    {
      words.push_back(token);
      ++count;
    }

    start = (end >= s.length()) ? std::string::npos : end + 1;
  }
  return count;
}

#ifdef _WIN32
uint32_t Logme::GetCurrentProcessId()
{
  return ::GetCurrentProcessId();
}

uint64_t Logme::GetCurrentThreadId()
{
  return (uint64_t)::GetCurrentThreadId();
}   
#endif

#if defined(__APPLE__) || defined(__linux__) || defined(__sun__) 

uint32_t Logme::GetCurrentProcessId()
{
  static uint64_t pid = getpid();
  return pid;
}

uint64_t Logme::GetCurrentThreadId()
{
  static thread_local uint64_t tid = 0;
  if (tid != 0)
  {
    return tid;
  }

#if defined(__APPLE__)
  uint64_t appleTid = 0;
  pthread_threadid_np(nullptr, &appleTid);
  tid = appleTid;
#elif defined(__linux__)
  tid = (uint64_t)syscall(SYS_gettid);
#else
  // Fallback: use pthread id cast.
  tid = (uint64_t)(uintptr_t)pthread_self();
#endif

  return tid;
}   

#endif

std::string Logme::TrimSpaces(const std::string& str)
{
  std::string v(str);

  constexpr const char* spaces{ " \t\n\r\f\v" };
  v.erase(v.find_last_not_of(spaces) + 1);
  v.erase(0, v.find_first_not_of(spaces));
  return v;
}

std::string& Logme::ToLowerAsciiInplace(std::string& v)
{
  std::transform(v.begin(), v.end(), v.begin(), [](char c) {
    return ::tolower((unsigned char)c);
  });

  return v;
}

std::string Logme::ReplaceAll(
  const std::string& where
  , const std::string& what
  , const std::string& on
)
{
  if (what == on)
    return where;

  std::string str(where);

  while (true)
  {
    auto pos = str.find(what);
    if (pos == std::string::npos)
      break;

    str = str.substr(0, pos) + on + str.substr(pos + what.size());
  }

  return str;
}

std::string Logme::Join(const StringArray& arr, const std::string separator)
{
  std::string result;
  for (const auto& item : arr)
  {
    if (!result.empty())
      result += separator;

    result += item;
  }
  return result;
}

std::string Logme::GetLevelName(Logme::Level level)
{
  switch (level)
  {
  case Logme::Level::LEVEL_DEBUG: return "DEBUG";
  case Logme::Level::LEVEL_INFO: return "INFO";
  case Logme::Level::LEVEL_WARN: return "WARN";
  case Logme::Level::LEVEL_ERROR: return "ERROR";
  case Logme::Level::LEVEL_CRITICAL: return "CRITICAL";
  default: return "UNKNOWN";
  }
}