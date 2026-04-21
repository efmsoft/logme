#include <Common/TestBackend.h>

#include <gtest/gtest.h>

#include <Logme/Logme.h>
#include <Logme/Procedure.h>

std::shared_ptr<TestBackend> Be;
std::shared_ptr<TestBackend> ExtraBe;
Logme::ID TestCh{"preprocessor_compat"};

static int ProcedureWithReturn()
{
  int result = 42;
  LogmePI(result);
  return result;
}

#define _LogmePV1() \
  static Logme::ContextCache LOGME_JOIN(_logme_ctx_, __LINE__); \
  const Logme::Context& _procContext = \
    Logme::Context( \
      LOGME_JOIN(_logme_ctx_, __LINE__), \
      Logme::Level::LEVEL_CRITICAL, \
      &CH, \
      &SUBSID, \
      __FUNCTION__, \
      __FILE__, \
      __LINE__, \
      Logme::Context::Params()); \
  Logme::Procedure logme_proc(_procContext, nullptr)

static void ProcedureVoid()
{
  _LogmePV1();
}

TEST(PreprocessorCompat, MacrosBuildAndRunWithoutConformingPreprocessor)
{
  auto defaultCh = Logme::Instance->GetExistingChannel(CH);
  auto extraCh = Logme::Instance->CreateChannel(TestCh);

  defaultCh->SetFilterLevel(Logme::LEVEL_DEBUG);
  extraCh->SetFilterLevel(Logme::LEVEL_DEBUG);

  defaultCh->RemoveBackends();
  extraCh->RemoveBackends();

  Be = std::make_shared<TestBackend>(defaultCh);
  ExtraBe = std::make_shared<TestBackend>(extraCh);
  defaultCh->AddBackend(Be);
  extraCh->AddBackend(ExtraBe);

  Logme::OutputFlags flags;
  flags.Value = 0;
  defaultCh->SetFlags(flags);
  extraCh->SetFlags(flags);

  Be->Clear();
  ExtraBe->Clear();

  LogmeI(TestCh, "compat info %d", 1);

  for (int i = 0; i < 3; ++i)
  {
    LogmeW_Once(TestCh, "compat once %d", i);
  }

#ifndef LOGME_DISABLE_STD_FORMAT
  fLogmeE("compat {}", 2);
#endif

  int result = ProcedureWithReturn();
  ProcedureVoid();

  EXPECT_EQ(result, 42);
  EXPECT_GE(Be->History.size() + ExtraBe->History.size(), 6u);

  size_t onceCount = 0;
  bool hasInfo = false;
  bool hasStdFormat = false;
  bool hasProcedureReturn = false;
  bool hasProcedureVoid = false;

  for (const auto& history : {&Be->History, &ExtraBe->History})
  {
    for (const auto& line : *history)
    {
      if (line.find("compat once") != std::string::npos)
      {
        onceCount++;
      }
      if (line.find("compat info 1") != std::string::npos)
      {
        hasInfo = true;
      }
#ifndef LOGME_DISABLE_STD_FORMAT
      if (line.find("compat 2") != std::string::npos)
      {
        hasStdFormat = true;
      }
#endif
      if (line.find("ProcedureWithReturn") != std::string::npos)
      {
        hasProcedureReturn = true;
      }
      if (line.find("ProcedureVoid") != std::string::npos)
      {
        hasProcedureVoid = true;
      }
    }
  }

  EXPECT_EQ(onceCount, 1u);
  EXPECT_TRUE(hasInfo);
#ifndef LOGME_DISABLE_STD_FORMAT
  EXPECT_TRUE(hasStdFormat);
#endif
  EXPECT_TRUE(hasProcedureReturn);
  EXPECT_TRUE(hasProcedureVoid);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  auto ch = Logme::Instance->GetExistingChannel(CH);
  ch->RemoveBackends();
  Be = std::make_shared<TestBackend>(ch);
  ch->AddBackend(Be);

  Logme::OutputFlags flags;
  flags.Value = 0;
  ch->SetFlags(flags);

  return RUN_ALL_TESTS();
}
