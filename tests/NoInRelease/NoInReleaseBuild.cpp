#include <Logme/Logme.h>

int main()
{
  LogmeI() << "Hello from stream API";
  LogmeI("Hello %s", "from printf API");
  return 0;
}
