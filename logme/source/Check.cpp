#include <Logme/Check.h>

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
#endif

#include <Logme/AllStatI.h>

namespace Logme
{
  namespace Detail
  {
    std::string CaptureSystemErrorText()
    {
      return std::string(OSERR2);
    }
  }
}
