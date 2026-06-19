#include <Logme/Backend/FileBackend.h>
#include <Logme/Logme.h>

#include <atomic>
#include <assert.h>
#include <string.h>
#include <thread>

#ifdef USE_JSONCPP
#include <json/json.h>
#endif

#define LOGME_GLOG_VLOG_LEVEL 1
#include <Logme/GlogCompat.h>

#if defined(_MSC_VER)
#pragma warning(disable: 4459)
#endif

static void OutputToDefaultChannel()
{
  LogmeI("Line1");
  LogmeE("Line %i", 2);
  LogmeC("Some string: %s", "string");

  using namespace Logme;

  auto ch = Instance->GetChannel(CH);
  OutputFlags flags = ch->GetFlags();
  flags.Location = DETALITY_SHORT;
  ch->SetFlags(flags);

  LogmeW("With brief file location");

  flags.Location = DETALITY_FULL;
  ch->SetFlags(flags);

  LogmeI("With full file location");

  flags.Location = DETALITY_NONE;
  ch->SetFlags(flags);
}

static std::string OutputToChannel1()
{
  std::string ret("str value");
  LogmeP(ret);

  LOGME_CHANNEL(CH1, "channel1");

  auto ch = Logme::Instance->CreateChannel(CH1);

  auto file = std::make_shared<Logme::FileBackend>(ch);
  file->CreateLog("OutputToChannel1.log");

  ch->AddBackend(file);

  LogmeW(CH1, "test test %i", 1);
  LogmeW_If(CH1.Name == nullptr, CH1, "disabled message");

  return ret;
}

static int OutputToNamespaceChannel(int arg1, const char* argv, const std::string& str)
{
  int rc = 8;
  LogmeP(rc, ARGS3(arg1, argv, str));

  LOGME_CHANNEL(CH, "notdefault");

  auto ch = Logme::Instance->CreateChannel(CH);

  auto file = std::make_shared<Logme::FileBackend>(ch);
  file->CreateLog("OutputToNamespaceChannel.log");

  ch->AddBackend(file);

  LogmeW("data written to \"%s\" channel", CH.Name);
  return rc;
}


#ifdef USE_JSONCPP
static void TestFileBackendConfigValidation()
{
  using namespace Logme;

  {
    Json::Value config;
    config["file"] = "invalid-rotation.log";
    config["rotation"] = "invalid-rotation";

    FileBackendConfig backendConfig;
    assert(!backendConfig.Parse(&config));
  }

  {
    struct TestCase
    {
      const char* Value;
      TimeRotationMode TimeRotation;
      bool DailyRotation;
    };

    const TestCase testCases[] =
    {
      { "hourly", TIME_ROTATION_HOURLY, false },
      { "daily", TIME_ROTATION_DAILY, true },
      { "weekly", TIME_ROTATION_WEEKLY, false },
      { "monthly", TIME_ROTATION_MONTHLY, false },
      { "none", TIME_ROTATION_NONE, false },
      { "off", TIME_ROTATION_NONE, false },
      { "disabled", TIME_ROTATION_NONE, false },
    };

    for (const TestCase& testCase : testCases)
    {
      Json::Value config;
      config["file"] = "valid-rotation.log";
      config["rotation"] = testCase.Value;

      FileBackendConfig backendConfig;
      assert(backendConfig.Parse(&config));
      assert(backendConfig.TimeRotation == testCase.TimeRotation);
      assert(backendConfig.DailyRotation == testCase.DailyRotation);
    }
  }

  {
    Json::Value config;
    config["file"] = "invalid-max-parts.log";
    config["max-parts"] = -1;

    FileBackendConfig backendConfig;
    assert(!backendConfig.Parse(&config));
  }

  {
    Json::Value config;
    config["file"] = "invalid-compression.log";
    config["compression"] = "zip";

    FileBackendConfig backendConfig;
    assert(!backendConfig.Parse(&config));
  }

  {
    Json::Value config;
    config["file"] = "valid-compression.log";
    config["rotation"] = "daily";
    config["compression"] = "gz";
    config["max-parts"] = 0;

    FileBackendConfig backendConfig;
    assert(backendConfig.Parse(&config));
    assert(backendConfig.DailyRotation);
    assert(backendConfig.GzipCompression);
    assert(backendConfig.MaxParts == 0);
  }
}
#endif

static void TestRetention()
{
  using namespace Logme;
  LOGME_CHANNEL(CH2, "channel2");

  OutputFlags flags;
  flags.Value = 0;

  auto ch = Instance->CreateChannel(CH2, flags);

  auto file = std::make_shared<FileBackend>(ch);

  const size_t maxSize = 4096;
  file->SetMaxSize(maxSize);

  const char* log = "TestRetention.log";

  remove(log);
  file->CreateLog(log);

  ch->AddBackend(file);
  
  const char* ipsum = 
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, \n"
    "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. \n"
    "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi \n"
    "ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit \n"
    "in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur \n"
    "sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt \n"
    "mollit anim id est laborum.\n";

  size_t size = 0;
  const char* t = ipsum;

  while (size < 5 * maxSize / 4)
  {
    const char* p = strchr(t, '\n');
    std::string line(t, p + 1 - t);
    t = p + 1;
    if (!strchr(t, '\n'))
      t = ipsum;

    LogmeI(CH2, "%s", line.c_str());
    size += line.length();
  }
}

enum class MyEnum
{
  MyEnumZero,
  MyEnumOne,
  MyEnumTwo,
};

template<> std::string FormatValue<MyEnum>(const MyEnum& value)
{
  switch (value)
  {
  case MyEnum::MyEnumZero: return "MyEnumZero";
  case MyEnum::MyEnumOne: return "MyEnumOne";
  case MyEnum::MyEnumTwo: return "MyEnumTwo";
  default:
    break;
  }
  return "";
}

static MyEnum ProcReturningEnumValue()
{
  MyEnum value{};
  LogmeP(value);

  std::this_thread::sleep_for(std::chrono::milliseconds(122));

  value = MyEnum::MyEnumTwo;
  return value;
}

static const char* CppOutput()
{
  const char* ret = "const string";
  LogmeP(ret);

  int value = 5;
  std::string str("std::string");
  LogmeI() << "value is " << value << ", ret=" << ret << ", str=" << str << Logme::ToStdString(L" wstring");

  return ret;
}

static void FormattedOutput()
{
#ifndef LOGME_DISABLE_STD_FORMAT
  fLogmeI("Some {} string. Integer: {}", "format", 123);
#endif
}

struct FlushTestBackend : public Logme::Backend
{
  std::atomic<int> DisplayCount;
  std::atomic<int> FlushCount;

  explicit FlushTestBackend(Logme::ChannelPtr owner)
    : Backend(owner, "FlushTestBackend")
    , DisplayCount(0)
    , FlushCount(0)
  {
  }

  void Display(Logme::Context& context) override
  {
    (void)context;
    DisplayCount.fetch_add(1, std::memory_order_relaxed);
  }

  void Flush() override
  {
    FlushCount.fetch_add(1, std::memory_order_relaxed);
  }
};


static void TestCheckAndCompatMacros()
{
  using namespace Logme;

  LOGME_CHANNEL(CH, "check-macros");

  auto ch = Instance->CreateChannel(CH);
  ch->RemoveBackends();
  ch->SetFilterLevel(LEVEL_DEBUG);

  auto backend = std::make_shared<FlushTestBackend>(ch);
  ch->AddBackend(backend);

  int expensive = 0;
  LogmeCheck(true) << ++expensive;
  assert(expensive == 0);
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == 0);

  LogmeCheck(false) << "native check";
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == 1);

  int leftCalls = 0;
  int rightCalls = 0;
  auto leftValue = [&]()
  {
    ++leftCalls;
    return 1;
  };
  auto rightValue = [&]()
  {
    ++rightCalls;
    return 2;
  };

  LogmeCheckEq(leftValue(), rightValue()) << "native check eq";
  assert(leftCalls == 1);
  assert(rightCalls == 1);
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == 2);

  int trueLeftCalls = 0;
  int trueRightCalls = 0;
  auto trueLeftValue = [&]()
  {
    ++trueLeftCalls;
    return 7;
  };
  auto trueRightValue = [&]()
  {
    ++trueRightCalls;
    return 7;
  };

  LogmeCheckEq(trueLeftValue(), trueRightValue()) << ++expensive;
  assert(trueLeftCalls == 1);
  assert(trueRightCalls == 1);
  assert(expensive == 0);
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == 2);

  int value = 7;
  int pointerCalls = 0;
  auto getPointer = [&]() -> int*
  {
    ++pointerCalls;
    return &value;
  };
  assert(LogmeCheckNotNull(getPointer()) == &value);
  assert(pointerCalls == 1);
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == 2);
  (void)getPointer;

  int nullPointerCalls = 0;
  auto getNullPointer = [&]() -> int*
  {
    ++nullPointerCalls;
    return nullptr;
  };
  assert(LogmeCheckNotNull(getNullPointer()) == nullptr);
  assert(nullPointerCalls == 1);
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == 3);

  LogmePCheck(true) << ++expensive;
  assert(expensive == 0);
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == 3);

  LogmePCheck(false) << "native pcheck";
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == 4);

  LogmeI_P() << "native plog";
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == 5);

  int firstNExpensive = 0;
  for (int i = 0; i < 3; ++i)
    LogmeI_FirstN(2) << ++firstNExpensive;
  assert(firstNExpensive == 2);
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == 7);

  int everyNExpensive = 0;
  for (int i = 0; i < 4; ++i)
    LogmeI_EveryN(2) << ++everyNExpensive;
  assert(everyNExpensive == 2);
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == 9);

  int everyNZeroExpensive = 0;
  LogmeI_EveryN(0) << ++everyNZeroExpensive;
  assert(everyNZeroExpensive == 0);
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == 9);

  LOG(INFO) << "compat log";
  LOG_IF(INFO, false) << ++expensive;
  assert(expensive == 0);
  LOG_IF(INFO, true) << "compat log if";
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == 11);

  CHECK_NE(1, 1) << "compat check";
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == 12);

  int compatLeftCalls = 0;
  int compatRightCalls = 0;
  auto compatLeftValue = [&]()
  {
    ++compatLeftCalls;
    return 1;
  };
  auto compatRightValue = [&]()
  {
    ++compatRightCalls;
    return 2;
  };

  CHECK_EQ(compatLeftValue(), compatRightValue()) << "compat check eq";
  assert(compatLeftCalls == 1);
  assert(compatRightCalls == 1);
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == 13);

  CHECK_EQ(5, 5) << ++expensive;
  assert(expensive == 0);
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == 13);

  PCHECK(true) << ++expensive;
  assert(expensive == 0);
  PCHECK(false) << "compat pcheck";
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == 14);

  PLOG(ERROR) << "compat plog";
  PLOG_IF(ERROR, false) << ++expensive;
  assert(expensive == 0);
  PLOG_IF(ERROR, true) << "compat plog if";
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == 16);

  LOG(FATAL) << "compat fatal without handler";
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == 17);

#ifndef NDEBUG
  DLOG(INFO) << "compat dlog";
  DLOG_IF(INFO, false) << ++expensive;
  assert(expensive == 0);
  DCHECK(false) << "compat dcheck";
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == 19);
#else
  DLOG(INFO) << ++expensive;
  DLOG_IF(INFO, true) << ++expensive;
  DCHECK(false) << ++expensive;
  assert(expensive == 0);
#endif

  VLOG(2) << ++expensive;
  assert(expensive == 0);
  VLOG(1) << "compat vlog";
  VLOG_IF(1, false) << ++expensive;
  assert(expensive == 0);
  VLOG_IF(1, true) << "compat vlog if";

  int expectedCount = 19;
#ifdef NDEBUG
  expectedCount = 17;
#endif
  expectedCount += 2;
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == expectedCount);

  int compatFirstNExpensive = 0;
  for (int i = 0; i < 3; ++i)
    LOG_FIRST_N(INFO, 2) << ++compatFirstNExpensive;
  assert(compatFirstNExpensive == 2);
  expectedCount += 2;
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == expectedCount);

  int compatEveryNExpensive = 0;
  for (int i = 0; i < 4; ++i)
    LOG_EVERY_N(INFO, 2) << ++compatEveryNExpensive;
  assert(compatEveryNExpensive == 2);
  expectedCount += 2;
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == expectedCount);
}

static void TestFatalHandler()
{
  using namespace Logme;

  LOGME_CHANNEL(FATAL_TEST_CH, "fatal-test");

  auto ch = Instance->CreateChannel(FATAL_TEST_CH);
  ch->RemoveBackends();
  ch->SetFilterLevel(LEVEL_DEBUG);

  auto backend = std::make_shared<FlushTestBackend>(ch);
  ch->AddBackend(backend);

  int handlerCalls = 0;
  Instance->SetFatalHandler([&]()
  {
    handlerCalls++;
    assert(backend->FlushCount.load(std::memory_order_relaxed) > 0);
    LogmeC(FATAL_TEST_CH, "nested critical");
  });

  LogmeC(FATAL_TEST_CH, "critical with handler");

  assert(handlerCalls == 1);
  assert(backend->DisplayCount.load(std::memory_order_relaxed) == 2);

  Instance->ResetFatalHandler();
  int flushCount = backend->FlushCount.load(std::memory_order_relaxed);
  (void)flushCount;

  LogmeC(FATAL_TEST_CH, "critical without handler");

  assert(handlerCalls == 1);
  assert(backend->FlushCount.load(std::memory_order_relaxed) == flushCount);
}

int main()
{
#ifdef USE_JSONCPP
  TestFileBackendConfigValidation();
#endif
  OutputToDefaultChannel();
  OutputToChannel1();
  OutputToNamespaceChannel(128, "hello world", std::string("std::string"));
  TestRetention();

  using namespace Logme;
  auto ch = Instance->GetChannel(CH);
  OutputFlags flags = ch->GetFlags();
  flags.Duration = true;
  ch->SetFlags(flags);

  ProcReturningEnumValue();
  CppOutput();
  FormattedOutput();
  TestFatalHandler();
  TestCheckAndCompatMacros();

  return 0;
}
