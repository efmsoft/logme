#define LOGME_INRELEASE
#include <Logme/Logme.h>
#include <Logme/Backend/FileBackend.h>

#include <string.h>
#include <thread>

void OutputToDefaultChannel()
{
  LogmeI("Line1");
  LogmeE("Line %i", 2);
  LogmeC("Some string: %s", "string");

  using namespace Logme;

  auto ch = Instance->GetChannel(CH);
  auto flags = ch->GetFlags();
  flags.Location = DETALITY_SHORT;
  ch->SetFlags(flags);

  LogmeW("With brief file location");

  flags.Location = DETALITY_FULL;
  ch->SetFlags(flags);

  LogmeI("With full file location");

  flags.Location = DETALITY_NONE;
  ch->SetFlags(flags);
}

std::string OutputToChannel1()
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

int OutputToNamespaceChannel(int arg1, const char* argv, const std::string& str)
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

void TestRetention()
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

MyEnum ProcReturningEnumValue()
{
  MyEnum value;
  LogmeP(value);

  std::this_thread::sleep_for(std::chrono::milliseconds(122));

  value = MyEnum::MyEnumTwo;
  return value;
}

const char* CppOutput()
{
  const char* ret = "const string";
  LogmeP(ret);

  int value = 5;
  std::string str("std::string");
  LogmeI() << "value is " << value << ", ret=" << ret << ", str=" << str << Logme::ToStdString(L" wstring");

  return ret;
}

void FormattedOutput()
{
  fLogmeI("Some {} string. Integer: {}", "format", 123);
}

int main()
{
  OutputToDefaultChannel();
  OutputToChannel1();
  OutputToNamespaceChannel(128, "hello world", std::string("std::string"));
  TestRetention();

  using namespace Logme;
  auto ch = Instance->GetChannel(CH);
  auto flags = ch->GetFlags();
  flags.Duration = true;
  ch->SetFlags(flags);

  ProcReturningEnumValue();
  CppOutput();
  FormattedOutput();

  return 0;
}
