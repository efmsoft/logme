#include <Logme/FastFormat.h>

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

namespace
{
  constexpr int FAST_FORMAT_CACHE_SIZE = 8;
  constexpr int FAST_FORMAT_SCAN_LIMIT = 48;

  enum class FastFormatKind : uint8_t
  {
    NONE,
    LITERAL,
    FORMAT_S,
    FORMAT_D,
    FORMAT_I
  };

  struct FastFormatEntry
  {
    const char* Format;
    FastFormatKind Kind;
    uint16_t PrefixLen;
    uint16_t SuffixPos;
    uint16_t SuffixLen;
  };

#if LOGME_FAST_FORMAT_STATS
  struct FastFormatStats
  {
    std::atomic<uint64_t> Calls{0};
    std::atomic<uint64_t> CacheHits{0};
    std::atomic<uint64_t> CacheMisses{0};
    std::atomic<uint64_t> CacheStores{0};
    std::atomic<uint64_t> DetectedNone{0};
    std::atomic<uint64_t> DetectedLiteral{0};
    std::atomic<uint64_t> DetectedS{0};
    std::atomic<uint64_t> DetectedD{0};
    std::atomic<uint64_t> DetectedI{0};
    std::atomic<uint64_t> ExecuteNone{0};
    std::atomic<uint64_t> ExecuteLiteral{0};
    std::atomic<uint64_t> ExecuteS{0};
    std::atomic<uint64_t> ExecuteD{0};
    std::atomic<uint64_t> ExecuteI{0};
    std::atomic<uint64_t> AnalyzeNullFormat{0};
    std::atomic<uint64_t> AnalyzeNoPercent{0};
    std::atomic<uint64_t> AnalyzeSecondPercent{0};
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
        " detected_none=%llu"
        " detected_literal=%llu"
        " detected_s=%llu"
        " detected_d=%llu"
        " detected_i=%llu"
        " execute_none=%llu"
        " execute_literal=%llu"
        " execute_s=%llu"
        " execute_d=%llu"
        " execute_i=%llu"
        " analyze_null_format=%llu"
        " analyze_no_percent=%llu"
        " analyze_second_percent=%llu"
        " analyze_unsupported_spec=%llu"
        " analyze_spec_at_end=%llu"
        " analyze_too_long=%llu"
        "\n",
        (unsigned long long)Stats.Calls.load(std::memory_order_relaxed),
        (unsigned long long)Stats.CacheHits.load(std::memory_order_relaxed),
        (unsigned long long)Stats.CacheMisses.load(std::memory_order_relaxed),
        (unsigned long long)Stats.CacheStores.load(std::memory_order_relaxed),
        (unsigned long long)Stats.DetectedNone.load(std::memory_order_relaxed),
        (unsigned long long)Stats.DetectedLiteral.load(std::memory_order_relaxed),
        (unsigned long long)Stats.DetectedS.load(std::memory_order_relaxed),
        (unsigned long long)Stats.DetectedD.load(std::memory_order_relaxed),
        (unsigned long long)Stats.DetectedI.load(std::memory_order_relaxed),
        (unsigned long long)Stats.ExecuteNone.load(std::memory_order_relaxed),
        (unsigned long long)Stats.ExecuteLiteral.load(std::memory_order_relaxed),
        (unsigned long long)Stats.ExecuteS.load(std::memory_order_relaxed),
        (unsigned long long)Stats.ExecuteD.load(std::memory_order_relaxed),
        (unsigned long long)Stats.ExecuteI.load(std::memory_order_relaxed),
        (unsigned long long)Stats.AnalyzeNullFormat.load(std::memory_order_relaxed),
        (unsigned long long)Stats.AnalyzeNoPercent.load(std::memory_order_relaxed),
        (unsigned long long)Stats.AnalyzeSecondPercent.load(std::memory_order_relaxed),
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

  inline void StoreFastFormat(const FastFormatEntry& entry)
  {
    FastFormatCache[FastFormatCachePos] = entry;
    FastFormatCachePos = (FastFormatCachePos + 1) % FAST_FORMAT_CACHE_SIZE;
    FAST_FORMAT_STAT(CacheStores);
  }

  inline FastFormatEntry* FindFastFormat(const char* format)
  {
    for (auto& entry : FastFormatCache)
    {
      if (entry.Format == format)
        return &entry;
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
    auto rc = std::to_chars(temp, temp + sizeof(temp), value);
    if (rc.ec != std::errc())
      return;

    CopyBounded(dst, left, temp, (size_t)(rc.ptr - temp));
  }

  inline void AddDetectStat(FastFormatKind kind)
  {
    switch (kind)
    {
    case FastFormatKind::NONE:
      FAST_FORMAT_STAT(DetectedNone);
      break;

    case FastFormatKind::LITERAL:
      FAST_FORMAT_STAT(DetectedLiteral);
      break;

    case FastFormatKind::FORMAT_S:
      FAST_FORMAT_STAT(DetectedS);
      break;

    case FastFormatKind::FORMAT_D:
      FAST_FORMAT_STAT(DetectedD);
      break;

    case FastFormatKind::FORMAT_I:
      FAST_FORMAT_STAT(DetectedI);
      break;
    }
  }

  FastFormatEntry BuildFastFormat(const char* format)
  {
    FastFormatEntry entry{};
    entry.Format = format;
    entry.Kind = FastFormatKind::NONE;

    if (format == nullptr)
    {
      FAST_FORMAT_STAT(AnalyzeNullFormat);
      AddDetectStat(entry.Kind);
      return entry;
    }

    int percentPos = -1;

    for (int i = 0; i < FAST_FORMAT_SCAN_LIMIT; i++)
    {
      char ch = format[i];

      if (ch == '\0')
      {
        if (percentPos == -1)
        {
          entry.Kind = FastFormatKind::LITERAL;
          entry.PrefixLen = (uint16_t)i;
          FAST_FORMAT_STAT(AnalyzeNoPercent);
        }
        else
        {
          entry.SuffixLen = (uint16_t)(i - entry.SuffixPos);
        }

        AddDetectStat(entry.Kind);
        return entry;
      }

      if (ch != '%')
        continue;

      if (percentPos != -1)
      {
        FAST_FORMAT_STAT(AnalyzeSecondPercent);
        AddDetectStat(entry.Kind);
        return entry;
      }

      percentPos = i;

      if (i + 1 >= FAST_FORMAT_SCAN_LIMIT)
      {
        FAST_FORMAT_STAT(AnalyzeTooLong);
        AddDetectStat(entry.Kind);
        return entry;
      }

      char spec = format[i + 1];

      if (spec == '\0')
      {
        FAST_FORMAT_STAT(AnalyzeSpecAtEnd);
        AddDetectStat(entry.Kind);
        return entry;
      }

      if (spec == 's')
        entry.Kind = FastFormatKind::FORMAT_S;
      else if (spec == 'd')
        entry.Kind = FastFormatKind::FORMAT_D;
      else if (spec == 'i')
        entry.Kind = FastFormatKind::FORMAT_I;
      else
      {
        entry.Kind = FastFormatKind::NONE;
        FAST_FORMAT_STAT(AnalyzeUnsupportedSpec);
        AddDetectStat(entry.Kind);
        return entry;
      }

      entry.PrefixLen = (uint16_t)i;
      entry.SuffixPos = (uint16_t)(i + 2);
    }

    FAST_FORMAT_STAT(AnalyzeTooLong);
    AddDetectStat(entry.Kind);
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

    switch (entry.Kind)
    {
    case FastFormatKind::NONE:
      FAST_FORMAT_STAT(ExecuteNone);
      return false;

    case FastFormatKind::LITERAL:
      FAST_FORMAT_STAT(ExecuteLiteral);
      CopyBounded(dst, left, entry.Format, entry.PrefixLen);
      return true;

    case FastFormatKind::FORMAT_S:
    {
      FAST_FORMAT_STAT(ExecuteS);
      const char* text = va_arg(args, const char*);
      if (text == nullptr)
        text = "(null)";

      CopyBounded(dst, left, entry.Format, entry.PrefixLen);
      CopyBounded(dst, left, text, BoundedStringLen(text, left > 0 ? left - 1 : 0));
      CopyBounded(dst, left, entry.Format + entry.SuffixPos, entry.SuffixLen);
      return true;
    }

    case FastFormatKind::FORMAT_D:
    case FastFormatKind::FORMAT_I:
    {
      if (entry.Kind == FastFormatKind::FORMAT_D)
        FAST_FORMAT_STAT(ExecuteD);
      else
        FAST_FORMAT_STAT(ExecuteI);

      int value = va_arg(args, int);

      CopyBounded(dst, left, entry.Format, entry.PrefixLen);
      AppendInt(dst, left, value);
      CopyBounded(dst, left, entry.Format + entry.SuffixPos, entry.SuffixLen);
      return true;
    }
    }

    return false;
  }
}

bool Logme::TryFastFormat(
  char* buffer
  , size_t bufferSize
  , const char* format
  , va_list args
)
{
  FAST_FORMAT_STAT(Calls);

  if (format == nullptr || buffer == nullptr || bufferSize == 0)
    return false;

  FastFormatEntry* cached = FindFastFormat(format);
  if (cached)
  {
    FAST_FORMAT_STAT(CacheHits);
    return ExecuteFastFormat(*cached, buffer, bufferSize, args);
  }

  FAST_FORMAT_STAT(CacheMisses);

  FastFormatEntry entry = BuildFastFormat(format);
  StoreFastFormat(entry);

  return ExecuteFastFormat(entry, buffer, bufferSize, args);
}
