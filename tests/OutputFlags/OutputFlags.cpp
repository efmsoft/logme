#include <gtest/gtest.h>
#include <Logme/Logme.h>
#include <Logme/Backend/Backend.h>
#include <Logme/Utils.h>

#include <string>

using namespace Logme;

struct TestBackend : public Backend
{
  std::string Line;

  TestBackend(ChannelPtr owner)
    : Backend(owner)
  {
  }

  void Clear()
  {
    Line.clear();
  }

  void Display(Context& context, const char* line) override
  {
    auto flags = Owner->GetFlags();

    int nc;
    Line = context.Apply(flags, line, nc);
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

  const char* text = "lorem ipsum";

  Be->Clear();
  LogmeE(CHT, "%s %i", text, 3);
  EXPECT_EQ(Be->Line, std::string("E {") + CHT.Name + "} " + text + " 3");
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

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  auto ch = Instance->CreateChannel(CHT);
  Be = std::make_shared<TestBackend>(ch);
  ch->SetFilterLevel(LEVEL_DEBUG);
  ch->AddBackend(Be);

  return RUN_ALL_TESTS();
}