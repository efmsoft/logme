#include <Logme/Logme.h>

static void FillBuffer(unsigned char* buffer, size_t size)
{
  const char* text1 =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. ";

  const char* text2 =
    "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. ";

  const char* text3 =
    "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris.";

  size_t pos = 0;

  // Copy first text block
  size_t len1 = strlen(text1);
  for (size_t i = 0; i < len1 && pos < size; i++, pos++)
  {
    buffer[pos] = (unsigned char)text1[i];
  }

  // Insert special / binary block
  for (size_t i = 0; i < 32 && pos < size; i++, pos++)
  {
    if (i % 5 == 0)
    {
      buffer[pos] = 0;              // zero byte
      continue;
    }

    if (i % 7 == 0)
    {
      buffer[pos] = 0xFF;           // non-printable
      continue;
    }

    buffer[pos] = (unsigned char)(i + 1); // small binary values
  }

  // Copy second text block
  size_t len2 = strlen(text2);
  for (size_t i = 0; i < len2 && pos < size; i++, pos++)
  {
    buffer[pos] = (unsigned char)text2[i];
  }

  // Another special characters block
  const char* special = "!@#$%^&*()_+-=[]{}|;':,.<>/?`~";
  size_t lenS = strlen(special);

  for (size_t i = 0; i < lenS && pos < size; i++, pos++)
  {
    buffer[pos] = (unsigned char)special[i];
  }

  // Copy final text block
  size_t len3 = strlen(text3);
  for (size_t i = 0; i < len3 && pos < size; i++, pos++)
  {
    buffer[pos] = (unsigned char)text3[i];
  }

  // Fill remaining with pattern
  for (; pos < size; pos++)
  {
    buffer[pos] = (unsigned char)('.'); // visible filler
  }
}

int main()
{
  unsigned char buffer[100];
  FillBuffer(buffer, sizeof(buffer));

  LogmeI("C-style, offset=0, lineLimit=8\n%s",
    Logme::DumpBuffer(buffer, sizeof(buffer), 0, 8).c_str()
  );

  LogmeI("C-style, offset=4, lineLimit=8\n%s",
    Logme::DumpBuffer(buffer, sizeof(buffer), 4, 8).c_str()
  );

#ifndef LOGME_DISABLE_STD_FORMAT
  fLogmeI("fmt-style, offset=0, lineLimit=8\n{}",
    Logme::DumpBuffer(buffer, sizeof(buffer), 0, 8)
  );

  fLogmeI("fmt-style, offset=8, lineLimit=4\n{}",
    Logme::DumpBuffer(buffer, sizeof(buffer), 8, 4)
  );
#endif

  return 0;
}
