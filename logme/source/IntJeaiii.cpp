#include <stddef.h>
#include <stdint.h>

#include <Logme/Utils.h>

int Logme::PrintIntJeaiii(
  char* buffer
  , size_t size
  , int value
)
{
  static const char DIGITS[201] =
    "00010203040506070809"
    "10111213141516171819"
    "20212223242526272829"
    "30313233343536373839"
    "40414243444546474849"
    "50515253545556575859"
    "60616263646566676869"
    "70717273747576777879"
    "80818283848586878889"
    "90919293949596979899";

  if (buffer == nullptr || size == 0)
    return -1;

  char* p = buffer;
  uint32_t u;

  if (value < 0)
  {
    if (size < 2)
      return -1;

    *p++ = '-';
    u = (uint32_t)(-(int64_t)value);
    size--;
  }
  else
  {
    u = (uint32_t)value;
  }

  // --- FAST PATH: very small numbers ---
  if (u < 100)
  {
    if (u < 10)
    {
      if (size < 2)
        return -1;

      p[0] = (char)('0' + u);
      p[1] = 0;
      return (int)(p - buffer + 1);
    }

    if (size < 3)
      return -1;

    p[0] = DIGITS[u * 2];
    p[1] = DIGITS[u * 2 + 1];
    p[2] = 0;
    return (int)(p - buffer + 2);
  }

  if (u < 10000)
  {
    int len = (u < 1000) ? 3 : 4;

    if (size <= (size_t)len)
      return -1;

    char* q = p + len;
    uint32_t v = u;

    uint32_t r = v % 100;
    v /= 100;

    q -= 2;
    q[0] = DIGITS[r * 2];
    q[1] = DIGITS[r * 2 + 1];

    if (v < 10)
    {
      *--q = (char)('0' + v);
    }
    else
    {
      q -= 2;
      q[0] = DIGITS[v * 2];
      q[1] = DIGITS[v * 2 + 1];
    }

    int total = (int)(p + len - buffer);
    buffer[total] = 0;
    return total;
  }

  // --- general path ---
  int len = 1;
  if (u >= 1000000000) len = 10;
  else if (u >= 100000000) len = 9;
  else if (u >= 10000000) len = 8;
  else if (u >= 1000000) len = 7;
  else if (u >= 100000) len = 6;
  else if (u >= 10000) len = 5;

  if (size <= (size_t)len)
    return -1;

  char* out = p + len;
  char* q = out;
  uint32_t v = u;

  while (v >= 100)
  {
    uint32_t q2 = v / 100;
    uint32_t r = v - q2 * 100;

    q -= 2;
    q[0] = DIGITS[r * 2];
    q[1] = DIGITS[r * 2 + 1];

    v = q2;
  }

  if (v < 10)
  {
    *--q = (char)('0' + v);
  }
  else
  {
    q -= 2;
    q[0] = DIGITS[v * 2];
    q[1] = DIGITS[v * 2 + 1];
  }

  int total = (int)(out - buffer);
  buffer[total] = 0;

  return total;
}