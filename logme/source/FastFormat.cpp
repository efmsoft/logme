#include <Logme/Context.h>
#include <Logme/FastFormat.h>
#include <Logme/Utils.h>

#include <charconv>
#include <cstdint>
#include <cstring>

#ifndef LOGME_FAST_FORMAT_STATS
#define LOGME_FAST_FORMAT_STATS 0
#endif

#if LOGME_FAST_FORMAT_STATS
#include <atomic>
#include <cstdio>
#endif

#define LOGME_INT2STR_JEAIII 0

using namespace Logme;

namespace
{
  constexpr int FAST_FORMAT_CACHE_SIZE = 2;
  constexpr int FAST_FORMAT_SCAN_LIMIT = 48;

#if LOGME_FAST_FORMAT_STATS
  struct FastFormatStats
  {
    std::atomic<uint64_t> Calls{0};
    std::atomic<uint64_t> CacheHits{0};
    std::atomic<uint64_t> CacheMisses{0};
    std::atomic<uint64_t> CacheStores{0};
    std::atomic<uint64_t> DetectedRejected{0};
    std::atomic<uint64_t> DetectedLiteral{0};
    std::atomic<uint64_t> DetectedOneSpec{0};
    std::atomic<uint64_t> DetectedTwoSpecs{0};
    std::atomic<uint64_t> DetectedS{0};
    std::atomic<uint64_t> DetectedD{0};
    std::atomic<uint64_t> DetectedI{0};
    std::atomic<uint64_t> DetectedU{0};
    std::atomic<uint64_t> DetectedP{0};
    std::atomic<uint64_t> ExecuteRejected{0};
    std::atomic<uint64_t> ExecuteLiteral{0};
    std::atomic<uint64_t> ExecuteOneSpec{0};
    std::atomic<uint64_t> ExecuteTwoSpecs{0};
    std::atomic<uint64_t> ExecuteS{0};
    std::atomic<uint64_t> ExecuteD{0};
    std::atomic<uint64_t> ExecuteI{0};
    std::atomic<uint64_t> ExecuteU{0};
    std::atomic<uint64_t> ExecuteP{0};
    std::atomic<uint64_t> AnalyzeNullFormat{0};
    std::atomic<uint64_t> AnalyzeNoPercent{0};
    std::atomic<uint64_t> AnalyzeMoreThanTwoSpecs{0};
    std::atomic<uint64_t> AnalyzeUnsupportedSpec{0};
    std::atomic<uint64_t> AnalyzeSpecAtEnd{0};
    std::atomic<uint64_t> AnalyzeTooLong{0};
  };

  FastFormatStats Stats;

  inline void AddStat(std::atomic<uint64_t>& value)
  {
    value.fetch_add(1, std::memory_order_relaxed);
  }

  struct FastFormatStatsPrinter
  {
    ~FastFormatStatsPrinter()
    {
      fprintf(
        stderr,
        "[logme][FastFormat]"
        " calls=%llu"
        " hits=%llu"
        " misses=%llu"
        " stores=%llu"
        " detected_rejected=%llu"
        " detected_literal=%llu"
        " detected_one_spec=%llu"
        " detected_two_specs=%llu"
        " detected_s=%llu"
        " detected_d=%llu"
        " detected_i=%llu"
        " detected_u=%llu"
        " detected_p=%llu"
        " execute_rejected=%llu"
        " execute_literal=%llu"
        " execute_one_spec=%llu"
        " execute_two_specs=%llu"
        " execute_s=%llu"
        " execute_d=%llu"
        " execute_i=%llu"
        " execute_u=%llu"
        " execute_p=%llu"
        " analyze_null_format=%llu"
        " analyze_no_percent=%llu"
        " analyze_more_than_two_specs=%llu"
        " analyze_unsupported_spec=%llu"
        " analyze_spec_at_end=%llu"
        " analyze_too_long=%llu"
        "\n",
        (unsigned long long)Stats.Calls.load(std::memory_order_relaxed),
        (unsigned long long)Stats.CacheHits.load(std::memory_order_relaxed),
        (unsigned long long)Stats.CacheMisses.load(std::memory_order_relaxed),
        (unsigned long long)Stats.CacheStores.load(std::memory_order_relaxed),
        (unsigned long long)Stats.DetectedRejected.load(std::memory_order_relaxed),
        (unsigned long long)Stats.DetectedLiteral.load(std::memory_order_relaxed),
        (unsigned long long)Stats.DetectedOneSpec.load(std::memory_order_relaxed),
        (unsigned long long)Stats.DetectedTwoSpecs.load(std::memory_order_relaxed),
        (unsigned long long)Stats.DetectedS.load(std::memory_order_relaxed),
        (unsigned long long)Stats.DetectedD.load(std::memory_order_relaxed),
        (unsigned long long)Stats.DetectedI.load(std::memory_order_relaxed),
        (unsigned long long)Stats.DetectedU.load(std::memory_order_relaxed),
        (unsigned long long)Stats.DetectedP.load(std::memory_order_relaxed),
        (unsigned long long)Stats.ExecuteRejected.load(std::memory_order_relaxed),
        (unsigned long long)Stats.ExecuteLiteral.load(std::memory_order_relaxed),
        (unsigned long long)Stats.ExecuteOneSpec.load(std::memory_order_relaxed),
        (unsigned long long)Stats.ExecuteTwoSpecs.load(std::memory_order_relaxed),
        (unsigned long long)Stats.ExecuteS.load(std::memory_order_relaxed),
        (unsigned long long)Stats.ExecuteD.load(std::memory_order_relaxed),
        (unsigned long long)Stats.ExecuteI.load(std::memory_order_relaxed),
        (unsigned long long)Stats.ExecuteU.load(std::memory_order_relaxed),
        (unsigned long long)Stats.ExecuteP.load(std::memory_order_relaxed),
        (unsigned long long)Stats.AnalyzeNullFormat.load(std::memory_order_relaxed),
        (unsigned long long)Stats.AnalyzeNoPercent.load(std::memory_order_relaxed),
        (unsigned long long)Stats.AnalyzeMoreThanTwoSpecs.load(std::memory_order_relaxed),
        (unsigned long long)Stats.AnalyzeUnsupportedSpec.load(std::memory_order_relaxed),
        (unsigned long long)Stats.AnalyzeSpecAtEnd.load(std::memory_order_relaxed),
        (unsigned long long)Stats.AnalyzeTooLong.load(std::memory_order_relaxed)
      );
    }
  };

  FastFormatStatsPrinter StatsPrinter;

#define FAST_FORMAT_STAT(x) AddStat(Stats.x)
#else
#define FAST_FORMAT_STAT(x) do { } while (0)
#endif

  thread_local FastFormatEntry FastFormatCache[FAST_FORMAT_CACHE_SIZE]{};
  thread_local uint32_t FastFormatCachePos = 0;
  std::mutex FastCacheLock;

  inline void StoreFastFormat(const FastFormatEntry& entry)
  {
    FastFormatCache[FastFormatCachePos] = entry;
    FastFormatCachePos = (FastFormatCachePos + 1) % FAST_FORMAT_CACHE_SIZE;
    FAST_FORMAT_STAT(CacheStores);
  }

  inline FastFormatEntry* FindFastFormat(ContextCache& cache, const char* format)
  {
    if (cache.State.load(std::memory_order_acquire) == ContextCacheState::READY)
    {
      if (cache.Ffe.Format == format)
        return &cache.Ffe;

      cache.State.store(ContextCacheState::DISABLED, std::memory_order_release);
    }

    for (auto& entry : FastFormatCache)
    {
      if (entry.Format == format)
      {
        if (cache.State.load(std::memory_order_acquire) == ContextCacheState::EMPTY)
        { 
          std::lock_guard guard(FastCacheLock);

          if (cache.State.load(std::memory_order_relaxed) == ContextCacheState::EMPTY)
          { 
            cache.Ffe = entry;
            cache.State.store(ContextCacheState::READY, std::memory_order_release);
          }
        }

        return &entry;
      }
    }

    return nullptr;
  }

  inline size_t BoundedStringLen(const char* text, size_t limit)
  {
    size_t len = 0;
    while (len < limit && text[len] != '\0')
      len++;

    return len;
  }

  inline void CopyBounded(
    char*& dst
    , size_t& left
    , const char* src
    , size_t len
  )
  {
    if (left <= 1 || len == 0)
      return;

    size_t n = len;
    if (n >= left)
      n = left - 1;

    memcpy(dst, src, n);
    dst += n;
    left -= n;
    *dst = '\0';
  }

  inline void AppendInt(
    char*& dst
    , size_t& left
    , int value
  )
  {
    if (left <= 1)
      return;

    char temp[32];

#if LOGME_INT2STR_JEAIII
    int len = Logme::PrintIntJeaiii(
      temp
      , sizeof(temp)
      , value
    );
    if (len < 0)
      return;
    
    CopyBounded(dst, left, temp, (size_t)len);
#else
    auto rc = std::to_chars(temp, temp + sizeof(temp), value);
    if (rc.ec != std::errc())
      return;
    
    CopyBounded(dst, left, temp, (size_t)(rc.ptr - temp));
#endif
  }

  inline void AddDetectSpecStat(FastFormatKind kind)
  {
    switch (kind)
    {
    case FastFormatKind::FORMAT_S:
      FAST_FORMAT_STAT(DetectedS);
      break;

    case FastFormatKind::FORMAT_D:
      FAST_FORMAT_STAT(DetectedD);
      break;

    case FastFormatKind::FORMAT_I:
      FAST_FORMAT_STAT(DetectedI);
      break;

    case FastFormatKind::FORMAT_U:
      FAST_FORMAT_STAT(DetectedU);
      break;

    case FastFormatKind::FORMAT_P:
      FAST_FORMAT_STAT(DetectedP);
      break;

    default:
      break;
    }
  }

  inline void AddExecuteSpecStat(FastFormatKind kind)
  {
    switch (kind)
    {
    case FastFormatKind::FORMAT_S:
      FAST_FORMAT_STAT(ExecuteS);
      break;

    case FastFormatKind::FORMAT_D:
      FAST_FORMAT_STAT(ExecuteD);
      break;

    case FastFormatKind::FORMAT_I:
      FAST_FORMAT_STAT(ExecuteI);
      break;

    case FastFormatKind::FORMAT_U:
      FAST_FORMAT_STAT(ExecuteU);
      break;

    case FastFormatKind::FORMAT_P:
      FAST_FORMAT_STAT(ExecuteP);
      break;

    default:
      break;
    }
  }

  template<typename TArgs>
  inline bool AppendArg(
    FastFormatKind kind
    , char*& dst
    , size_t& left
    , TArgs& args
  )
  {
    switch (kind)
    {
    case FastFormatKind::FORMAT_S:
    {
      const char* text = va_arg(args, const char*);
      if (text == nullptr)
        text = "(null)";

      CopyBounded(dst, left, text, BoundedStringLen(text, left > 0 ? left - 1 : 0));
      return true;
    }

    case FastFormatKind::FORMAT_D:
    case FastFormatKind::FORMAT_I:
    {
      int value = va_arg(args, int);
      AppendInt(dst, left, value);
      return true;
    }

    case FastFormatKind::FORMAT_U:
    {
      unsigned int value = va_arg(args, unsigned int);

      char temp[32];
      auto rc = std::to_chars(temp, temp + sizeof(temp), value);
      if (rc.ec != std::errc())
        return false;

      CopyBounded(dst, left, temp, (size_t)(rc.ptr - temp));
      return true;
    }

    case FastFormatKind::FORMAT_P:
    {
      void* value = va_arg(args, void*);

      char temp[32];
      temp[0] = '0';
      temp[1] = 'x';
      auto rc = std::to_chars(
        temp + 2
        , temp + sizeof(temp)
        , (uintptr_t)value
        , 16
      );
      if (rc.ec != std::errc())
        return false;

      CopyBounded(dst, left, temp, (size_t)(rc.ptr - temp));
      return true;
    }

    default:
      return false;
    }
  }

  FastFormatEntry BuildFastFormat(const char* format)
  {
    FastFormatEntry entry{};
    entry.Format = format;
    entry.Kind1 = FastFormatKind::NONE;
    entry.Kind2 = FastFormatKind::NONE;
    entry.SpecCount = 0;

    if (format == nullptr)
    {
      FAST_FORMAT_STAT(AnalyzeNullFormat);
      FAST_FORMAT_STAT(DetectedRejected);
      return entry;
    }

    auto formatLen = strlen(format);
    if (formatLen >= FAST_FORMAT_SCAN_LIMIT)
    {
      FAST_FORMAT_STAT(AnalyzeTooLong);
      FAST_FORMAT_STAT(DetectedRejected);
      return entry;
    }

    for (size_t i = 0; i < formatLen; i++)
    {
      char ch = format[i];

      if (ch != '%')
        continue;

      if (i + 1 >= formatLen)
      {
        FAST_FORMAT_STAT(AnalyzeSpecAtEnd);
        FAST_FORMAT_STAT(DetectedRejected);
        entry.SpecCount = 0;
        entry.Kind1 = FastFormatKind::NONE;
        entry.Kind2 = FastFormatKind::NONE;
        return entry;
      }

      char spec = format[i + 1];

      FastFormatKind kind = FastFormatKind::NONE;
      if (spec == 's')
        kind = FastFormatKind::FORMAT_S;
      else if (spec == 'd')
        kind = FastFormatKind::FORMAT_D;
      else if (spec == 'i')
        kind = FastFormatKind::FORMAT_I;
      else if (spec == 'u')
        kind = FastFormatKind::FORMAT_U;
      else if (spec == 'p')
        kind = FastFormatKind::FORMAT_P;
      else
      {
        FAST_FORMAT_STAT(AnalyzeUnsupportedSpec);
        FAST_FORMAT_STAT(DetectedRejected);
        entry.SpecCount = 0;
        entry.Kind1 = FastFormatKind::NONE;
        entry.Kind2 = FastFormatKind::NONE;
        return entry;
      }

      if (entry.SpecCount == 0)
      {
        entry.Kind1 = kind;
        entry.Part0Len = (uint16_t)i;
        entry.Part1Pos = (uint16_t)(i + 2);
        entry.SpecCount = 1;
      }
      else if (entry.SpecCount == 1)
      {
        entry.Kind2 = kind;
        entry.Part1Len = (uint16_t)(i - entry.Part1Pos);
        entry.Part2Pos = (uint16_t)(i + 2);
        entry.SpecCount = 2;
      }
      else
      {
        FAST_FORMAT_STAT(AnalyzeMoreThanTwoSpecs);
        FAST_FORMAT_STAT(DetectedRejected);
        entry.SpecCount = 0;
        entry.Kind1 = FastFormatKind::NONE;
        entry.Kind2 = FastFormatKind::NONE;
        return entry;
      }

      i++;
    }

    if (entry.SpecCount == 0)
    {
      entry.Kind1 = FastFormatKind::LITERAL;
      entry.Part0Len = (uint16_t)formatLen;
      FAST_FORMAT_STAT(AnalyzeNoPercent);
      FAST_FORMAT_STAT(DetectedLiteral);
      return entry;
    }

    if (entry.SpecCount == 1)
      entry.Part1Len = (uint16_t)(formatLen - entry.Part1Pos);
    else
      entry.Part2Len = (uint16_t)(formatLen - entry.Part2Pos);

    if (entry.SpecCount == 1)
      FAST_FORMAT_STAT(DetectedOneSpec);
    else
      FAST_FORMAT_STAT(DetectedTwoSpecs);

    AddDetectSpecStat(entry.Kind1);
    if (entry.SpecCount == 2)
      AddDetectSpecStat(entry.Kind2);

    return entry;
  }

  bool ExecuteFastFormat(
    const FastFormatEntry& entry
    , char* buffer
    , size_t bufferSize
    , va_list args
  )
  {
    if (buffer == nullptr || bufferSize == 0)
      return false;

    char* dst = buffer;
    size_t left = bufferSize;

    *dst = '\0';

    if (entry.Kind1 == FastFormatKind::NONE)
    {
      FAST_FORMAT_STAT(ExecuteRejected);
      return false;
    }

    if (entry.Kind1 == FastFormatKind::LITERAL)
    {
      FAST_FORMAT_STAT(ExecuteLiteral);
      CopyBounded(dst, left, entry.Format, entry.Part0Len);
      return true;
    }

    if (entry.SpecCount == 1)
      FAST_FORMAT_STAT(ExecuteOneSpec);
    else if (entry.SpecCount == 2)
      FAST_FORMAT_STAT(ExecuteTwoSpecs);

    AddExecuteSpecStat(entry.Kind1);
    CopyBounded(dst, left, entry.Format, entry.Part0Len);
    if (AppendArg(entry.Kind1, dst, left, args) == false)
      return false;

    if (entry.SpecCount == 1)
    {
      CopyBounded(dst, left, entry.Format + entry.Part1Pos, entry.Part1Len);
      return true;
    }

    if (entry.SpecCount != 2)
    {
      FAST_FORMAT_STAT(ExecuteRejected);
      return false;
    }

    CopyBounded(dst, left, entry.Format + entry.Part1Pos, entry.Part1Len);
    AddExecuteSpecStat(entry.Kind2);
    if (AppendArg(entry.Kind2, dst, left, args) == false)
      return false;

    CopyBounded(dst, left, entry.Format + entry.Part2Pos, entry.Part2Len);
    return true;
  }
}

bool Logme::TryFastFormat(
  Context& context
  , char* buffer
  , size_t bufferSize
  , const char* format
  , va_list args
)
{
  FAST_FORMAT_STAT(Calls);

  if (format == nullptr || buffer == nullptr || bufferSize == 0)
    return false;

  FastFormatEntry* cached = FindFastFormat(context.Cache, format);
  if (cached)
  {
    FAST_FORMAT_STAT(CacheHits);
    return ExecuteFastFormat(*cached, buffer, bufferSize, args);
  }

  FAST_FORMAT_STAT(CacheMisses);

  FastFormatEntry entry = BuildFastFormat(format);
  StoreFastFormat(entry);

  if (context.Cache.State.load(std::memory_order_acquire) == ContextCacheState::EMPTY)
  { 
    std::lock_guard guard(FastCacheLock);
    if (context.Cache.State.load(std::memory_order_relaxed) == ContextCacheState::EMPTY)
    {
      context.Cache.Ffe = entry;
      context.Cache.State.store(ContextCacheState::READY, std::memory_order_release);
    }
  }

  return ExecuteFastFormat(entry, buffer, bufferSize, args);
}
