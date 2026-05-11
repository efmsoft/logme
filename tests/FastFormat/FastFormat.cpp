#include <Common/TestBackend.h>

#include <gtest/gtest.h>

#include <Logme/Context.h>
#include <Logme/FastFormat.h>
#include <Logme/Logme.h>

#include <cstdarg>
#include <cstring>
#include <memory>
#include <string>

Logme::ID CHT{ "fast_format" };
std::shared_ptr<TestBackend> Be;

static bool TryFormat(
  Logme::ContextCache& cache
  , const char* format
  , char* buffer
  , size_t bufferSize
  , size_t* outLen
  , ...
)
{
  Logme::Context context(cache, Logme::LEVEL_INFO, &CHT, nullptr);

  va_list args;
  va_start(args, outLen);
  bool rc = Logme::TryFastFormat(
    context
    , buffer
    , bufferSize
    , format
    , args
    , outLen
  );
  va_end(args);

  return rc;
}

static void ExpectReady(const Logme::ContextCache& cache)
{
  EXPECT_EQ(
    cache.State.load(std::memory_order_acquire)
    , Logme::ContextCacheState::READY
  );
}

static void LogBoundedThenString(int fd, const char* file)
{
  LogmeI(CHT, "fd=%d file=%s", fd, file);
}

static void LogStringThenBounded(const char* file, int fd)
{
  LogmeI(CHT, "file=%s fd=%d", file, fd);
}

TEST(FastFormat, LiteralUsesFastPathAndBufferSizeHint)
{
  Logme::ContextCache cache;
  char buffer[128]{};
  size_t outLen = 0;

  bool rc = TryFormat(
    cache
    , "literal text"
    , buffer
    , sizeof(buffer)
    , &outLen
  );

  EXPECT_TRUE(rc);
  EXPECT_STREQ(buffer, "literal text");
  EXPECT_EQ(outLen, std::strlen("literal text"));
  ExpectReady(cache);
  EXPECT_EQ(cache.Ffe.Kind1, Logme::FastFormatKind::LITERAL);
  EXPECT_EQ(cache.Ffe.SpecCount, 0);
  EXPECT_GT(cache.Ffe.BufferSizeHint, 0);
}

TEST(FastFormat, BoundedOneSpecifierUsesFastPathAndBufferSizeHint)
{
  Logme::ContextCache cache;
  char buffer[128]{};
  size_t outLen = 0;

  bool rc = TryFormat(
    cache
    , "value=%d"
    , buffer
    , sizeof(buffer)
    , &outLen
    , 42
  );

  EXPECT_TRUE(rc);
  EXPECT_STREQ(buffer, "value=42");
  EXPECT_EQ(outLen, std::strlen("value=42"));
  ExpectReady(cache);
  EXPECT_EQ(cache.Ffe.Kind1, Logme::FastFormatKind::FORMAT_D);
  EXPECT_EQ(cache.Ffe.SpecCount, 1);
  EXPECT_GT(cache.Ffe.BufferSizeHint, 0);
}

TEST(FastFormat, BoundedTwoSpecifiersUseFastPathAndBufferSizeHint)
{
  Logme::ContextCache cache;
  char buffer[128]{};
  size_t outLen = 0;

  bool rc = TryFormat(
    cache
    , "a=%d b=%u"
    , buffer
    , sizeof(buffer)
    , &outLen
    , -7
    , 77u
  );

  EXPECT_TRUE(rc);
  EXPECT_STREQ(buffer, "a=-7 b=77");
  EXPECT_EQ(outLen, std::strlen("a=-7 b=77"));
  ExpectReady(cache);
  EXPECT_EQ(cache.Ffe.Kind1, Logme::FastFormatKind::FORMAT_D);
  EXPECT_EQ(cache.Ffe.Kind2, Logme::FastFormatKind::FORMAT_U);
  EXPECT_EQ(cache.Ffe.SpecCount, 2);
  EXPECT_GT(cache.Ffe.BufferSizeHint, 0);
}

TEST(FastFormat, StringOneSpecifierUsesFastPathWithoutBufferSizeHint)
{
  Logme::ContextCache cache;
  char buffer[128]{};
  size_t outLen = 0;

  bool rc = TryFormat(
    cache
    , "file=%s"
    , buffer
    , sizeof(buffer)
    , &outLen
    , "test.log"
  );

  EXPECT_TRUE(rc);
  EXPECT_STREQ(buffer, "file=test.log");
  EXPECT_EQ(outLen, std::strlen("file=test.log"));
  ExpectReady(cache);
  EXPECT_EQ(cache.Ffe.Kind1, Logme::FastFormatKind::FORMAT_S);
  EXPECT_EQ(cache.Ffe.SpecCount, 1);
  EXPECT_EQ(cache.Ffe.BufferSizeHint, 0);
}

TEST(FastFormat, BoundedThenStringUsesFastPathWithoutBufferSizeHint)
{
  Logme::ContextCache cache;
  char buffer[128]{};
  size_t outLen = 0;

  bool rc = TryFormat(
    cache
    , "fd=%d file=%s"
    , buffer
    , sizeof(buffer)
    , &outLen
    , 3
    , "test.log"
  );

  EXPECT_TRUE(rc);
  EXPECT_STREQ(buffer, "fd=3 file=test.log");
  EXPECT_EQ(outLen, std::strlen("fd=3 file=test.log"));
  ExpectReady(cache);
  EXPECT_EQ(cache.Ffe.Kind1, Logme::FastFormatKind::FORMAT_D);
  EXPECT_EQ(cache.Ffe.Kind2, Logme::FastFormatKind::FORMAT_S);
  EXPECT_EQ(cache.Ffe.SpecCount, 2);
  EXPECT_EQ(cache.Ffe.BufferSizeHint, 0);
}

TEST(FastFormat, StringThenBoundedUsesFastPathWithoutBufferSizeHint)
{
  Logme::ContextCache cache;
  char buffer[128]{};
  size_t outLen = 0;

  bool rc = TryFormat(
    cache
    , "file=%s fd=%d"
    , buffer
    , sizeof(buffer)
    , &outLen
    , "test.log"
    , 3
  );

  EXPECT_TRUE(rc);
  EXPECT_STREQ(buffer, "file=test.log fd=3");
  EXPECT_EQ(outLen, std::strlen("file=test.log fd=3"));
  ExpectReady(cache);
  EXPECT_EQ(cache.Ffe.Kind1, Logme::FastFormatKind::FORMAT_S);
  EXPECT_EQ(cache.Ffe.Kind2, Logme::FastFormatKind::FORMAT_D);
  EXPECT_EQ(cache.Ffe.SpecCount, 2);
  EXPECT_EQ(cache.Ffe.BufferSizeHint, 0);
}

TEST(FastFormat, MoreThanTwoSpecifiersDisableFastPath)
{
  Logme::ContextCache cache;
  char buffer[128]{};
  size_t outLen = 0;

  bool rc = TryFormat(
    cache
    , "a=%d b=%d c=%d"
    , buffer
    , sizeof(buffer)
    , &outLen
    , 1
    , 2
    , 3
  );

  EXPECT_FALSE(rc);
  EXPECT_STREQ(buffer, "");
  EXPECT_EQ(outLen, 0u);
  ExpectReady(cache);
  EXPECT_EQ(cache.Ffe.Kind1, Logme::FastFormatKind::NONE);
  EXPECT_EQ(cache.Ffe.SpecCount, 0);
  EXPECT_EQ(cache.Ffe.BufferSizeHint, 0);
}

TEST(FastFormat, UnsupportedSpecifierDisablesFastPath)
{
  Logme::ContextCache cache;
  char buffer[128]{};
  size_t outLen = 0;

  bool rc = TryFormat(
    cache
    , "value=%lld"
    , buffer
    , sizeof(buffer)
    , &outLen
    , 42LL
  );

  EXPECT_FALSE(rc);
  EXPECT_STREQ(buffer, "");
  EXPECT_EQ(outLen, 0u);
  ExpectReady(cache);
  EXPECT_EQ(cache.Ffe.Kind1, Logme::FastFormatKind::NONE);
  EXPECT_EQ(cache.Ffe.SpecCount, 0);
  EXPECT_EQ(cache.Ffe.BufferSizeHint, 0);
}

TEST(FastFormat, LongFormatDisablesFastPath)
{
  Logme::ContextCache cache;
  char buffer[128]{};
  size_t outLen = 0;

  bool rc = TryFormat(
    cache
    , "012345678901234567890123456789012345678901234567"
    , buffer
    , sizeof(buffer)
    , &outLen
  );

  EXPECT_FALSE(rc);
  EXPECT_STREQ(buffer, "");
  EXPECT_EQ(outLen, 0u);
  ExpectReady(cache);
  EXPECT_EQ(cache.Ffe.Kind1, Logme::FastFormatKind::NONE);
  EXPECT_EQ(cache.Ffe.SpecCount, 0);
  EXPECT_EQ(cache.Ffe.BufferSizeHint, 0);
}

TEST(FastFormat, LoggerCacheHitDoesNotTruncateBoundedThenString)
{
  Logme::OutputFlags flags;
  flags.Value = 0;
  Be->Owner->SetFlags(flags);

  const char* file = "/opt/nfs/var/log/nfs_2026_05_10_19_50_23.log";

  Be->Clear();
  LogBoundedThenString(3, file);
  EXPECT_EQ(Be->Line, std::string("fd=3 file=") + file);

  Be->Clear();
  LogBoundedThenString(4, file);
  EXPECT_EQ(Be->Line, std::string("fd=4 file=") + file);
}

TEST(FastFormat, LoggerCacheHitDoesNotTruncateStringThenBounded)
{
  Logme::OutputFlags flags;
  flags.Value = 0;
  Be->Owner->SetFlags(flags);

  const char* file = "/opt/nfs/var/log/rdsman_2026_05_10_19_50_23.log";

  Be->Clear();
  LogStringThenBounded(file, 3);
  EXPECT_EQ(Be->Line, std::string("file=") + file + " fd=3");

  Be->Clear();
  LogStringThenBounded(file, 4);
  EXPECT_EQ(Be->Line, std::string("file=") + file + " fd=4");
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  auto ch = Logme::Instance->CreateChannel(CHT);
  ch->RemoveBackends();
  Be = std::make_shared<TestBackend>(ch);
  ch->AddBackend(Be);

  Logme::OutputFlags flags;
  flags.Value = 0;
  ch->SetFlags(flags);
  ch->SetFilterLevel(Logme::LEVEL_DEBUG);

  return RUN_ALL_TESTS();
}
