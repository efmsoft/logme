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

std::string Logme::dword2str(uint32_t value)
{
  char buff[32];
  snprintf(buff, sizeof(buff), "%u", (unsigned)value);

  return std::string(buff);
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
  return (uint64_t)pthread_self();
}   

#endif
