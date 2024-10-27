#include <algorithm>
#include <Logme/Utils.h>

#if defined(__GNUC__) && !defined(__DJGPP__)
#include <pthread.h> 
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
  return getpid();
}

uint64_t Logme::GetCurrentThreadId()
{
  return (uint64_t)gettid();
}   

#endif
