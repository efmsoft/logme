#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

// Compile-time obfuscation for format strings passed to logging macros.
//
// Usage:
//   LogmeI(OBF("Hello %s"), "world");
//
// Override base key in your project before including Logme headers:
//   #define LOGME_OBFSTR_KEY 0x12345678u
//
// Disable completely:
//   #define LOGME_DISABLE_OBFSTR

#ifndef LOGME_OBFSTR_KEY
  #define LOGME_OBFSTR_KEY 0xC3A5F1D7u
#endif

namespace Logme
{
  namespace ObfFmt
  {
    constexpr uint32_t MixKey(uint32_t v)
    {
      // xorshift32
      v ^= (v << 13);
      v ^= (v >> 17);
      v ^= (v << 5);
      return v;
    }

    constexpr uint32_t MakeKey(uint32_t line, uint32_t counter)
    {
      uint32_t k = static_cast<uint32_t>(LOGME_OBFSTR_KEY);
      k ^= 0x9E3779B9u * (line + 1u);
      k ^= 0x85EBCA6Bu * (counter + 1u);
      k = MixKey(k);
      return k ? k : 1u;
    }

    constexpr uint8_t KeyByte(uint32_t key, size_t i)
    {
      uint32_t v = key;
      v ^= static_cast<uint32_t>(i) * 0x9E3779B9u;
      v = MixKey(v);
      return static_cast<uint8_t>((v >> ((i & 3u) * 8u)) & 0xFFu);
    }

    template<size_t N, uint32_t KEY>
    class Str
    {
      std::array<char, N> Enc;
      mutable std::array<char, N> Dec;
      mutable bool Ready;

      static consteval std::array<char, N> Encrypt(const char (&s)[N])
      {
        std::array<char, N> out{};
        for (size_t i = 0; i < N; i++)
        {
          out[i] = static_cast<char>(s[i] ^ static_cast<char>(KeyByte(KEY, i)));
        }
        return out;
      }

      void Decrypt() const
      {
        if (Ready)
          return;

        for (size_t i = 0; i < N; i++)
        {
          Dec[i] = static_cast<char>(Enc[i] ^ static_cast<char>(KeyByte(KEY, i)));
        }

        Ready = true;
      }

    public:
      consteval explicit Str(const char (&s)[N])
        : Enc(Encrypt(s))
        , Dec{}
        , Ready(false)
      {
      }

      const char* CStr() const
      {
        Decrypt();
        return Dec.data();
      }

      operator const char*() const
      {
        return CStr();
      }
    };

    template<size_t N, uint32_t LINE, uint32_t COUNTER>
    consteval auto Make(const char (&s)[N])
    {
      return Str<N, MakeKey(LINE, COUNTER)>(s);
    }
  }
}

#ifndef OBF
#ifndef LOGME_DISABLE_OBFSTR
  #ifndef __LOGME_COUNTER__
    #define __LOGME_COUNTER__ 0
  #endif
  #define OBF(str) (::Logme::ObfFmt::Make<sizeof(str), __LINE__, __LOGME_COUNTER__>(str))
#else
  #define OBF(str) (str)
#endif
#endif
