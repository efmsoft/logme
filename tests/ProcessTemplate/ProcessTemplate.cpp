#include <gtest/gtest.h>

#include <Logme/Template.h>

#include <cctype>
#include <cstdint>
#include <regex>
#include <string>

static bool Contains(const std::string& str, const char* text)
{
  return str.find(text) != std::string::npos;
}

static bool StartsWith(const std::string& str, const char* text)
{
  return str.rfind(text, 0) == 0;
}

static bool EndsWith(const std::string& str, const char* text)
{
  size_t len = std::char_traits<char>::length(text);

  if (str.size() < len)
    return false;

  return str.compare(str.size() - len, len, text) == 0;
}

static bool IsDecimalNumber(const std::string& str)
{
  if (str.empty())
    return false;

  for (char ch : str)
  {
    if (!std::isdigit(static_cast<unsigned char>(ch)))
      return false;
  }

  return true;
}

TEST(ProcessTemplate, NullInputReturnsEmptyString)
{
  uint32_t notProcessed = 0xFFFFFFFF;

  std::string result = Logme::ProcessTemplate(
    nullptr
    , Logme::ProcessTemplateParam()
    , &notProcessed
  );

  EXPECT_TRUE(result.empty());
  EXPECT_EQ(notProcessed, 0U);
}

TEST(ProcessTemplate, LiteralWithoutTemplateFieldsIsUnchanged)
{
  uint32_t notProcessed = 0xFFFFFFFF;

  std::string result = Logme::ProcessTemplate(
    "plain/path/file.log"
    , Logme::ProcessTemplateParam()
    , &notProcessed
  );

  EXPECT_EQ(result, "plain/path/file.log");
  EXPECT_EQ(notProcessed, 0U);
}

TEST(ProcessTemplate, UnknownAndIncompleteFieldsArePreserved)
{
  uint32_t notProcessed = 0;

  std::string result = Logme::ProcessTemplate(
    "before-{unknown}-{pid-after"
    , Logme::ProcessTemplateParam()
    , &notProcessed
  );

  EXPECT_EQ(result, "before-{unknown}-{pid-after");
  EXPECT_EQ(notProcessed, 0U);
}

TEST(ProcessTemplate, PidIsReplacedWithDecimalValue)
{
  uint32_t notProcessed = 0;

  std::string result = Logme::ProcessTemplate(
    "pid-{pid}"
    , Logme::ProcessTemplateParam(Logme::TEMPLATE_PID)
    , &notProcessed
  );

  ASSERT_TRUE(StartsWith(result, "pid-"));
  EXPECT_TRUE(IsDecimalNumber(result.substr(4)));
  EXPECT_FALSE(Contains(result, "{pid}"));
  EXPECT_EQ(notProcessed, 0U);
}

TEST(ProcessTemplate, TargetUsesProvidedChannel)
{
  Logme::ProcessTemplateParam param(Logme::TEMPLATE_TARGET);
  param.TargetChannel = "ACCESS";

  uint32_t notProcessed = 0;
  std::string result = Logme::ProcessTemplate(
    "target-{target}"
    , param
    , &notProcessed
  );

  EXPECT_EQ(result, "target-ACCESS");
  EXPECT_EQ(notProcessed, 0U);
}

TEST(ProcessTemplate, TargetUsesZeroWhenChannelIsNull)
{
  uint32_t notProcessed = 0;

  std::string result = Logme::ProcessTemplate(
    "target-{target}"
    , Logme::ProcessTemplateParam(Logme::TEMPLATE_TARGET)
    , &notProcessed
  );

  EXPECT_EQ(result, "target-0");
  EXPECT_EQ(notProcessed, 0U);
}

TEST(ProcessTemplate, DateAndDatetimeUseExpectedFormat)
{
  uint32_t notProcessed = 0;

  std::string date = Logme::ProcessTemplate(
    "{date}"
    , Logme::ProcessTemplateParam(Logme::TEMPLATE_PDATE)
    , &notProcessed
  );

  EXPECT_TRUE(std::regex_match(date, std::regex("[0-9]{4}-[0-9]{2}-[0-9]{2}")));
  EXPECT_EQ(notProcessed, 0U);

  std::string datetime = Logme::ProcessTemplate(
    "{datetime}"
    , Logme::ProcessTemplateParam(Logme::TEMPLATE_PDATETIME)
    , &notProcessed
  );

  EXPECT_TRUE(std::regex_match(datetime, std::regex("[0-9]{4}-[0-9]{2}-[0-9]{2}-[0-9]{2}-[0-9]{2}-[0-9]{2}")));
  EXPECT_EQ(notProcessed, 0U);
}

TEST(ProcessTemplate, ProcessNameAndExePathAreReplaced)
{
  uint32_t notProcessed = 0;

  std::string pname = Logme::ProcessTemplate(
    "a-{pname}-b"
    , Logme::ProcessTemplateParam(Logme::TEMPLATE_PNAME)
    , &notProcessed
  );

  EXPECT_TRUE(StartsWith(pname, "a-"));
  EXPECT_TRUE(EndsWith(pname, "-b"));
  EXPECT_GT(pname.size(), 4U);
  EXPECT_FALSE(Contains(pname, "{pname}"));
  EXPECT_EQ(notProcessed, 0U);

  std::string exepath = Logme::ProcessTemplate(
    "a-{exepath}-b"
    , Logme::ProcessTemplateParam(Logme::TEMPLATE_EXEPATH)
    , &notProcessed
  );

  EXPECT_TRUE(StartsWith(exepath, "a-"));
  EXPECT_TRUE(EndsWith(exepath, "-b"));
  EXPECT_GT(exepath.size(), 4U);
  EXPECT_FALSE(Contains(exepath, "{exepath}"));
  EXPECT_EQ(notProcessed, 0U);
}

TEST(ProcessTemplate, DisabledFieldsArePreservedAndReported)
{
  uint32_t notProcessed = 0;

  std::string result = Logme::ProcessTemplate(
    "{pid}-{pname}-{date}-{datetime}-{target}-{exepath}"
    , Logme::ProcessTemplateParam(0)
    , &notProcessed
  );

  EXPECT_EQ(result, "{pid}-{pname}-{date}-{datetime}-{target}-{exepath}");
  EXPECT_NE(notProcessed & Logme::TEMPLATE_PID, 0U);
  EXPECT_NE(notProcessed & Logme::TEMPLATE_PNAME, 0U);
  EXPECT_NE(notProcessed & Logme::TEMPLATE_PDATE, 0U);
  EXPECT_NE(notProcessed & Logme::TEMPLATE_PDATETIME, 0U);
  EXPECT_NE(notProcessed & Logme::TEMPLATE_TARGET, 0U);
  EXPECT_NE(notProcessed & Logme::TEMPLATE_EXEPATH, 0U);
}

TEST(ProcessTemplate, EnvironmentVariableIsReplaced)
{
  Logme::EnvSetVar("LOGME_TEMPLATE_TEST", "env-value");
  uint32_t notProcessed = 0;

  std::string result = Logme::ProcessTemplate(
    "before-{%LOGME_TEMPLATE_TEST}-after"
    , Logme::ProcessTemplateParam()
    , &notProcessed
  );

  EXPECT_EQ(result, "before-env-value-after");
  EXPECT_EQ(notProcessed, 0U);

  Logme::EnvSetVar("LOGME_TEMPLATE_TEST", nullptr);
}

TEST(ProcessTemplate, EnvironmentVariableValueIsProcessedRecursively)
{
  Logme::EnvSetVar("LOGME_TEMPLATE_RECURSIVE", "target-{target}");

  Logme::ProcessTemplateParam param(Logme::TEMPLATE_TARGET);
  param.TargetChannel = "HTTP";

  uint32_t notProcessed = 0;
  std::string result = Logme::ProcessTemplate(
    "{%LOGME_TEMPLATE_RECURSIVE}"
    , param
    , &notProcessed
  );

  EXPECT_EQ(result, "target-HTTP");
  EXPECT_EQ(notProcessed, 0U);

  Logme::EnvSetVar("LOGME_TEMPLATE_RECURSIVE", nullptr);
}

TEST(ProcessTemplate, EnvironmentReferenceWithPercentInNameIsPreserved)
{
  uint32_t notProcessed = 0;

  std::string result = Logme::ProcessTemplate(
    "{%BAD%NAME}"
    , Logme::ProcessTemplateParam()
    , &notProcessed
  );

  EXPECT_EQ(result, "{%BAD%NAME}");
  EXPECT_EQ(notProcessed, 0U);
}

TEST(ProcessTemplate, LongTemplateCrossesStackBufferAndKeepsResult)
{
  Logme::ProcessTemplateParam param(Logme::TEMPLATE_TARGET);
  param.TargetChannel = "CH";

  std::string input(600, 'x');
  input += "-{target}-";
  input += std::string(600, 'y');

  uint32_t notProcessed = 0;
  std::string result = Logme::ProcessTemplate(
    input.c_str()
    , param
    , &notProcessed
  );

  std::string expected(600, 'x');
  expected += "-CH-";
  expected += std::string(600, 'y');

  EXPECT_EQ(result, expected);
  EXPECT_EQ(notProcessed, 0U);
}
