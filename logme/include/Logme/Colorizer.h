#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>

namespace Logme
{
  class Colorizer
  {
  public:
    Colorizer(bool isStdErr);
   ~Colorizer();

    void Escape(const char* escape = 0);
    static bool ParseSequence(const char*& escape, int& attr, int& fg, int& bg);

#ifdef _WIN32
  private:

    HANDLE Output;
    CONSOLE_SCREEN_BUFFER_INFO Sbi;
    WORD LastAttr;
#else
  private:
    FILE* Stream;
#endif
  };
}
