#pragma once

#include <stddef.h>
#include <stdint.h>

#include <array>

// Compile-time string obfuscation for format strings.
//
// Usage:
//   LogmeI(OBF("Hello %s"), "world");
//
// Custom key (define before including Logme headers):
//   #define LOGME_OBFSTR_KEY 0x12345678u
//
// Disable obfuscation:
//   #define LOGME_DISABLE_OBFSTR

#ifndef LOGME_OBFSTR_KEY
  #define LOGME_OBFSTR_KEY 0xC3A5F1D7u
#endif

namespace Logme
{
  namespace Detail
  {
    constexpr uint32_t ObfMix(uint32_t v)
    {
      v ^= v >> 16;
      v *= 0x7FEB352Du;
      v ^= v >> 15;
      v *= 0x846CA68Bu;
      v ^= v >> 16;
      return v;
    }

    constexpr uint8_t ObfKeyByte(uint32_t key, size_t idx)
    {
      uint32_t x = ObfMix(key ^ static_cast<uint32_t>(idx * 0x9E3779B9u));
      return static_cast<uint8_t>(x & 0xFFu);
    }

    template<size_t N>
    class ObfString
    {
    public:
      consteval explicit ObfString(const char (&text)[N])
        : Enc{}
        , Dec{}
        , Decoded(false)
      {
        for (size_t i = 0; i < N; i++)
        {
          Enc[i] = static_cast<uint8_t>(text[i]) ^ ObfKeyByte(LOGME_OBFSTR_KEY, i);
        }
      }

      const char* CStr() const
      {
        if (!Decoded)
        {
          for (size_t i = 0; i < N; i++)
          {
            Dec[i] = static_cast<char>(Enc[i] ^ ObfKeyByte(LOGME_OBFSTR_KEY, i));
          }
          Decoded = true;
        }
        return Dec.data();
      }

      operator const char*() const
      {
        return CStr();
      }

    private:
      std::array<uint8_t, N> Enc;
      mutable std::array<char, N> Dec;
      mutable bool Decoded;
    };
  }

#ifndef LOGME_DISABLE_OBFSTR
  template<size_t N>
  consteval Detail::ObfString<N> MakeObfString(const char (&text)[N])
  {
    return Detail::ObfString<N>(text);
  }
#endif
}

#ifndef OBF
#ifndef LOGME_DISABLE_OBFSTR
  #define OBF(x) ::Logme::MakeObfString(x)
#else
  #define OBF(x) (x)
#endif
#endif
