#include <Logme/Logme.h>

int main()
{
  Logme::ID ch{"fmt"};

  auto channel = Logme::Instance->CreateChannel(ch);
  channel->AddLink(::CH);

#ifndef LOGME_DISABLE_STD_FORMAT
  fLogmeI("Hello {}, value={}", "format", 123);
  fLogmeW(ch, "Hex: 0x{:X}", 0xAB);
#else
  LogmeI() << "Hello " << "format" << ", value=" << 123;
  LogmeW(ch, "Hex: 0x%X", 0xAB);
#endif

  return 0;
}
