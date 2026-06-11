#include <Common/TestBackend.h>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 26495)
#endif

#include <gtest/gtest.h>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <Logme/Logme.h>

using namespace Logme;

namespace
{
  ID CHTF{ "thread_fields" };
  std::shared_ptr<TestBackend> Be;

  bool Contains(const std::string& text, const std::string& fragment)
  {
    return text.find(fragment) != std::string::npos;
  }

  void SetFormat(OutputFormat format)
  {
    OutputFlags flags;
    flags.Value = 0;
    flags.Format = format;
    Be->Owner->SetFlags(flags);
  }

  void ClearState()
  {
    Be->Clear();
    Logme::Instance->ClearThreadFields();
  }
}

TEST(ThreadField, JsonIncludesThreadFieldsFromMacro)
{
  ClearState();
  SetFormat(OUTPUT_JSON);

  ThreadFields fields;
  fields.Set("request_id", "request-42");
  fields.Set("tenant", "demo");

  {
    LogmeThreadFields(fields);
    LogmeI(CHTF, "json message");
  }

  EXPECT_TRUE(Contains(Be->Line, "\"request_id\":\"request-42\"")) << Be->Line;
  EXPECT_TRUE(Contains(Be->Line, "\"tenant\":\"demo\"")) << Be->Line;
  EXPECT_TRUE(Contains(Be->Line, "\"message\":\"json message\"")) << Be->Line;
}

TEST(ThreadField, XmlIncludesThreadFields)
{
  ClearState();
  SetFormat(OUTPUT_XML);

  Logme::Instance->SetThreadField("request_id", "request-43");
  Logme::Instance->SetThreadField("tenant", "demo");

  LogmeI(CHTF, "xml message");

  EXPECT_TRUE(Contains(Be->Line, "<request_id>request-43</request_id>")) << Be->Line;
  EXPECT_TRUE(Contains(Be->Line, "<tenant>demo</tenant>")) << Be->Line;
  EXPECT_TRUE(Contains(Be->Line, "<message>xml message</message>")) << Be->Line;
}

TEST(ThreadField, TextIgnoresThreadFields)
{
  ClearState();
  SetFormat(OUTPUT_TEXT);

  Logme::Instance->SetThreadField("request_id", "request-44");
  LogmeI(CHTF, "text message");

  EXPECT_EQ(Be->Line, "text message");
}

TEST(ThreadField, ThreadFieldsMacroRestoresPreviousFields)
{
  ClearState();
  SetFormat(OUTPUT_JSON);

  Logme::Instance->SetThreadField("request_id", "outer");

  ThreadFields fields;
  fields.Set("request_id", "inner");
  fields.Set("operation", "load-profile");

  {
    LogmeThreadFields(fields);
    LogmeI(CHTF, "inside");
  }

  EXPECT_TRUE(Contains(Be->History[0], "\"request_id\":\"inner\"")) << Be->History[0];
  EXPECT_TRUE(Contains(Be->History[0], "\"operation\":\"load-profile\"")) << Be->History[0];

  LogmeI(CHTF, "outside");

  EXPECT_TRUE(Contains(Be->History[1], "\"request_id\":\"outer\"")) << Be->History[1];
  EXPECT_FALSE(Contains(Be->History[1], "\"operation\"")) << Be->History[1];
}

TEST(ThreadField, RemoveAndClearThreadFields)
{
  ClearState();

  std::string value;
  Logme::Instance->SetThreadField("request_id", "request-45");
  EXPECT_TRUE(Logme::Instance->GetThreadField("request_id", value));
  EXPECT_EQ(value, "request-45");

  Logme::Instance->RemoveThreadField("request_id");
  EXPECT_FALSE(Logme::Instance->GetThreadField("request_id", value));

  Logme::Instance->SetThreadField("request_id", "request-46");
  Logme::Instance->SetThreadField("tenant", "demo");
  Logme::Instance->ClearThreadFields();
  EXPECT_FALSE(Logme::Instance->GetThreadField("request_id", value));
  EXPECT_FALSE(Logme::Instance->GetThreadField("tenant", value));
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  auto ch = Logme::Instance->CreateChannel(CHTF);
  Be = std::make_shared<TestBackend>(ch);
  ch->SetFilterLevel(LEVEL_DEBUG);
  ch->AddBackend(Be);

  return RUN_ALL_TESTS();
}
