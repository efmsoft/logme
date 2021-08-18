#include <gtest/gtest.h>

#include <Logme/Logme.h>
#include <Logme/Backend/Backend.h>
#include <Logme/Time/datetime.h>
#include <Logme/Utils.h>

#include <regex>
#include <string>
#include <vector>

using namespace Logme;

const char* Lorem = "lorem ipsum";

struct TestBackend : public Backend
{
  std::string Line;
  std::vector<std::string> History;

  TestBackend(ChannelPtr owner)
    : Backend(owner)
  {
  }

  void Clear()
  {
    Line.clear();
    History.clear();
  }

  void Display(Context& context, const char* line) override
  {
    auto flags = Owner->GetFlags();

    int nc;
    Line = context.Apply(flags, line, nc);
    History.push_back(Line);
  }
};

ID CHT{ "test_channel" };
std::shared_ptr<TestBackend> Be;

TEST(OutputFlags, Channel)
{
  OutputFlags flags;
  flags.Value = 0;
  flags.Signature = true;
  flags.Channel = true;
  Be->Owner->SetFlags(flags);

  Be->Clear();
  LogmeE(CHT, "%s %i", Lorem, 3);
  EXPECT_EQ(Be->Line, std::string("E {") + CHT.Name + "} " + Lorem + " 3");
}

long MyFunction(xint64_t p1, const char* p2)
{
  long rc = 8;
  LogmeP(rc, CHT, _ARGS2(p1, p2));

  Logme::Sleep(100);
  return rc;
}

TEST(OutputFlags, Duration)
{
  OutputFlags flags;
  flags.Value = 0;
  flags.Duration = true;
  Be->Owner->SetFlags(flags);

  xint64_t p1 = 0x2300000000LL;
  const char* p2 = "param";
  Be->Clear();

  std::stringstream ss;
  ss << ">> MyFunction(): p1=0x" << std::hex << p1 << ", p2=" << p2 << "";
  std::string line1 = ss.str();

  auto s = GetTimeInMillisec();
  auto rc = MyFunction(p1, p2);
  auto d = GetTimeInMillisec() - s;

  EXPECT_EQ(Be->History.size(), 2);
  EXPECT_EQ(Be->History[0], line1);

  ss.str("");
  ss << "<< MyFunction(): " << rc << " ";
  std::string prefix = ss.str();
  size_t pos = Be->History[1].find(prefix);
  EXPECT_EQ(pos, 0);

  std::string duration(Be->History[1].substr(prefix.length()));
  std::regex r("\\[(\\d+) ms\\]");

  std::smatch m;
  bool f = std::regex_match(duration, m, r);
  EXPECT_EQ(f, true);

  const int MAXDIFF = 5; // 5 ms
  int t = (unsigned int)atoi(m[1].str().c_str());
  int v = std::abs(int(d) - t);
  EXPECT_LE(v, MAXDIFF);
}

TEST(OutputFlags, Eol)
{
  OutputFlags flags;
  flags.Value = 0;
  flags.Eol = true;
  flags.Channel = true;
  Be->Owner->SetFlags(flags);

  Be->Clear();
  LogmeE(CHT, Lorem);
  EXPECT_EQ(Be->Line, std::string("{") + CHT.Name + "} " + Lorem + "\n");
}

TEST(OutputFlags, ErrorPrefix)
{
  OutputFlags flags;
  flags.Value = 0;
  flags.ErrorPrefix = true;
  Be->Owner->SetFlags(flags);

  Be->Clear();
  LogmeD(CHT, "_");
  EXPECT_EQ(Be->Line, "_");

  Be->Clear();
  LogmeI(CHT, "_");
  EXPECT_EQ(Be->Line, "_");

  Be->Clear();
  LogmeW(CHT, "_");
  EXPECT_EQ(Be->Line, "_");

  Be->Clear();
  LogmeE(CHT, "_");
  EXPECT_EQ(Be->Line, "Error: _");

  Be->Clear();
  LogmeC(CHT, "_");
  EXPECT_EQ(Be->Line, "Critical: _");
}

TEST(OutputFlags, Location)
{
  Module mod(__FILE__);

  OutputFlags flags;
  flags.Value = 0;
  Be->Owner->SetFlags(flags);

  Be->Clear();
  LogmeE(CHT, "_");
  EXPECT_EQ(Be->Line, "_");

  flags.Location = DETALITY_SHORT;
  Be->Owner->SetFlags(flags);
  auto line = __LINE__; LogmeE(CHT, "_");
  EXPECT_EQ(Be->Line, std::string(mod.ShortName) + "(" + std::to_string(line) + "): _");

  flags.Location = DETALITY_FULL;
  Be->Owner->SetFlags(flags);
  line = __LINE__; LogmeE(CHT, "_");
  EXPECT_EQ(Be->Line, std::string(mod.FullName) + "(" + std::to_string(line) + "): _");
}

void TestProc()
{
  LogmeI(CHT, "_");
}

TEST(OutputFlags, Method)
{
  OutputFlags flags;
  flags.Value = 0;
  flags.Method = true;
  Be->Owner->SetFlags(flags);

  Be->Clear();
  TestProc();
  EXPECT_EQ(Be->Line, std::string("TestProc(): _"));
}

TEST(OutputFlags, ProcessID)
{
  OutputFlags flags;
  flags.Value = 0;
  flags.ProcessID = true;
  Be->Owner->SetFlags(flags);

  const char* text = "noting";
  
  std::stringstream ss;
  ss << "[" << std::uppercase << std::hex << GetCurrentProcessId() << "] ";
  auto str = ss.str() + text;

  Be->Clear();
  LogmeI(CHT, "%s", text);
  EXPECT_EQ(Be->Line, str);
}

TEST(OutputFlags, ProcessAndThreadID)
{
  OutputFlags flags;
  flags.Value = 0;
  flags.ProcessID = true;
  flags.ThreadID = true;
  Be->Owner->SetFlags(flags);

  const char* text = "noting";

  std::stringstream ss;
  ss << "[" << std::uppercase << std::hex << GetCurrentProcessId() << ":" << GetCurrentThreadId() << "] ";
  auto str = ss.str() + text;

  Be->Clear();
  LogmeI(CHT, "%s", text);
  EXPECT_EQ(Be->Line, str);
}

TEST(OutputFlags, Signature)
{
  OutputFlags flags;
  flags.Value = 0;
  flags.Signature = true;
  Be->Owner->SetFlags(flags);

  const char* text = "something";

  Be->Clear();
  LogmeD(CHT, text);
  EXPECT_EQ(Be->Line, std::string("D ") + text);

  Be->Clear();
  LogmeI(CHT, text);
  EXPECT_EQ(Be->Line, std::string("  ") + text);

  Be->Clear();
  LogmeW(CHT, text);
  EXPECT_EQ(Be->Line, std::string("W ") + text);

  Be->Clear();
  LogmeE(CHT, text);
  EXPECT_EQ(Be->Line, std::string("E ") + text);

  Be->Clear();
  LogmeC(CHT, text);
  EXPECT_EQ(Be->Line, std::string("C ") + text);
}

TEST(OutputFlags, ThreadID)
{
  OutputFlags flags;
  flags.Value = 0;
  flags.ThreadID = true;
  Be->Owner->SetFlags(flags);

  const char* text = "bbb";

  std::stringstream ss;
  ss << "[:" << std::uppercase << std::hex << GetCurrentThreadId() << "] ";
  auto str = ss.str() + text;

  Be->Clear();
  LogmeI(CHT, "%s", text);
  EXPECT_EQ(Be->Line, str);
}

TEST(OutputFlags, Timestamp)
{
  OutputFlags flags;
  flags.Value = 0;
  Be->Owner->SetFlags(flags);

  Be->Clear();
  LogmeE(CHT, Lorem);
  EXPECT_EQ(Be->Line, Lorem);

  flags.Timestamp = TIME_FORMAT_LOCAL;
  Be->Owner->SetFlags(flags);
  LogmeE(CHT, "_");

  std::regex r("(\\d{4})-(\\d{2})-(\\d{2})\\s(\\d{2}):(\\d{2}):(\\d{2}):(\\d{3})\\s_");
  
  std::smatch m;
  bool f = std::regex_match(Be->Line, m, r);
  EXPECT_EQ(f, true);

  DateTime dt(
    DTK_LOCAL
    , atoi(m[1].str().c_str())
    , atoi(m[2].str().c_str())
    , atoi(m[3].str().c_str())
    , atoi(m[4].str().c_str())
    , atoi(m[5].str().c_str())
    , atoi(m[6].str().c_str())
    , atoi(m[7].str().c_str())
  );
  auto t = DateTime::Now();
  auto d = t - dt;
  const TicksType MAXDIFF = 100; // 100 ms
  EXPECT_LE(d, MAXDIFF);

  flags.Timestamp = TIME_FORMAT_TZ;
  Be->Owner->SetFlags(flags);
  LogmeE(CHT, "_");

  std::regex rtz("(\\d{4})-(\\d{2})-(\\d{2})T(\\d{2}):(\\d{2}):(\\d{2})(\\+|\\-)(\\d{2}):(\\d{2})\\s_");
  f = std::regex_match(Be->Line, m, rtz);
  EXPECT_EQ(f, true);

  flags.Timestamp = TIME_FORMAT_UTC;
  Be->Owner->SetFlags(flags);
  LogmeE(CHT, "_");

  f = std::regex_match(Be->Line, m, r);
  EXPECT_EQ(f, true);

  DateTime dtu(
    DTK_UTC
    , atoi(m[1].str().c_str())
    , atoi(m[2].str().c_str())
    , atoi(m[3].str().c_str())
    , atoi(m[4].str().c_str())
    , atoi(m[5].str().c_str())
    , atoi(m[6].str().c_str())
    , 0
  );
  t = DateTime::NowUtc();
  d = t - dtu;
  const TicksType MAXDIFFUTC = 2000; // 2 sec
  EXPECT_LE(d, MAXDIFFUTC);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  auto ch = Instance->CreateChannel(CHT);
  Be = std::make_shared<TestBackend>(ch);
  ch->SetFilterLevel(LEVEL_DEBUG);
  ch->AddBackend(Be);

  return RUN_ALL_TESTS();
}
