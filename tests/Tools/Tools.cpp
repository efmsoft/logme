#include <gtest/gtest.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
  namespace fs = std::filesystem;

  const char* FIXED_KEY_BASE64 = "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8=";

  static fs::path MakeTempDir(const char* name)
  {
    fs::path dir = fs::temp_directory_path() / "logme_tools_tests" / name;
    fs::remove_all(dir);
    fs::create_directories(dir);
    return dir;
  }

  static std::string Quote(const fs::path& path)
  {
    std::string text = path.string();
    std::string quoted = "\"";

    for (char c : text)
    {
      if (c == '"')
        quoted += "\\\"";
      else
        quoted.push_back(c);
    }

    quoted.push_back('"');
    return quoted;
  }

  static std::string QuoteText(const std::string& text)
  {
    std::string quoted = "\"";

    for (char c : text)
    {
      if (c == '"')
        quoted += "\\\"";
      else
        quoted.push_back(c);
    }

    quoted.push_back('"');
    return quoted;
  }

  static int Run(const std::string& command)
  {
    return std::system(command.c_str());
  }

  static void WriteText(const fs::path& path, const std::string& text)
  {
    std::ofstream out(path, std::ios::binary);
    ASSERT_TRUE(out);
    out << text;
  }

  static std::string ReadText(const fs::path& path)
  {
    std::ifstream in(path, std::ios::binary);
    EXPECT_TRUE(in);

    return std::string(
      std::istreambuf_iterator<char>(in)
      , std::istreambuf_iterator<char>()
    );
  }

  static std::vector<uint8_t> ReadBytes(const fs::path& path)
  {
    std::ifstream in(path, std::ios::binary);
    EXPECT_TRUE(in);

    return std::vector<uint8_t>(
      std::istreambuf_iterator<char>(in)
      , std::istreambuf_iterator<char>()
    );
  }

  static std::string Hex(const std::vector<uint8_t>& data)
  {
    static const char* hex = "0123456789ABCDEF";
    std::string out;
    out.reserve(data.size() * 2);

    for (uint8_t value : data)
    {
      out.push_back(hex[(value >> 4) & 0x0f]);
      out.push_back(hex[value & 0x0f]);
    }

    return out;
  }

  static std::string ToolCommand(const char* exe)
  {
    return Quote(fs::path(exe));
  }
}

TEST(LogmeFmtTool, ConvertsJsonToFinalizedXmlWithRenamedFields)
{
  fs::path dir = MakeTempDir("fmt_json_to_xml");
  fs::path input = dir / "input.jsonl";
  fs::path output = dir / "output.xml";

  WriteText(
    input
    , "{\"timestamp\":\"2026-05-03T01:02:03Z\",\"level\":\"INFO\",\"message\":\"hello\"}\n"
      "{\"timestamp\":\"2026-05-03T01:02:04Z\",\"level\":\"WARN\",\"message\":\"a<b & c\"}\n"
  );

  std::string command =
    ToolCommand(LOGMEFMT_EXE)
    + " --input json --output xml --finalize --root records --output-field message=msg --in "
    + Quote(input)
    + " --out "
    + Quote(output);

  ASSERT_EQ(0, Run(command));

  EXPECT_EQ(
    "<records>\n"
    "  <event><timestamp>2026-05-03T01:02:03Z</timestamp><level>INFO</level><msg>hello</msg></event>\n"
    "  <event><timestamp>2026-05-03T01:02:04Z</timestamp><level>WARN</level><msg>a&lt;b &amp; c</msg></event>\n"
    "</records>\n"
    , ReadText(output)
  );
}

TEST(LogmeFmtTool, ConvertsXmlToFinalizedJson)
{
  fs::path dir = MakeTempDir("fmt_xml_to_json");
  fs::path input = dir / "input.xml";
  fs::path output = dir / "output.json";

  WriteText(
    input
    , "<event><level>INFO</level><message>one</message></event>\n"
      "<event><level>ERROR</level><message>two</message></event>\n"
  );

  std::string command =
    ToolCommand(LOGMEFMT_EXE)
    + " --input xml --output json --finalize --in "
    + Quote(input)
    + " --out "
    + Quote(output);

  ASSERT_EQ(0, Run(command));

  EXPECT_EQ(
    "[\n"
    "  {\"level\":\"INFO\",\"message\":\"one\"},\n"
    "  {\"level\":\"ERROR\",\"message\":\"two\"}\n"
    "]\n"
    , ReadText(output)
  );
}

TEST(LogmeFmtTool, ConvertsTextToJsonLines)
{
  fs::path dir = MakeTempDir("fmt_text_to_json");
  fs::path input = dir / "input.log";
  fs::path output = dir / "output.jsonl";

  WriteText(input, "plain message\nsecond line\n");

  std::string command =
    ToolCommand(LOGMEFMT_EXE)
    + " --input text --output json --in "
    + Quote(input)
    + " --out "
    + Quote(output);

  ASSERT_EQ(0, Run(command));

  EXPECT_EQ(
    "{\"message\":\"plain message\"}\n"
    "{\"message\":\"second line\"}\n"
    , ReadText(output)
  );
}

TEST(LogmeObfTool, GeneratesKeyInAllFormats)
{
  fs::path dir = MakeTempDir("obf_generate_key");
  fs::path key = dir / "key.bin";
  fs::path header = dir / "key.h";
  fs::path base64 = dir / "key.txt";

  std::string command =
    ToolCommand(LOGMEOBF_EXE)
    + " --generate-key --key-out "
    + Quote(key)
    + " --header "
    + Quote(header)
    + " --name TEST_KEY --base64 > "
    + Quote(base64);

  ASSERT_EQ(0, Run(command));

  EXPECT_EQ(32u, ReadBytes(key).size());

  std::string headerText = ReadText(header);
  EXPECT_NE(std::string::npos, headerText.find("static const ObfKey TEST_KEY"));
  EXPECT_NE(std::string::npos, headerText.find("0x"));

  std::string base64Text = ReadText(base64);
  EXPECT_FALSE(base64Text.empty());
  EXPECT_EQ('\n', base64Text.back());
}

TEST(LogmeObfTool, ObfuscatesTextWithFixedGoldenOutput)
{
  fs::path dir = MakeTempDir("obf_text_golden");
  fs::path input = dir / "input.log";
  fs::path obf = dir / "output.obf";
  fs::path plain = dir / "plain.log";

  WriteText(
    input
    , "2026-05-03 01:02:03:004 I first line\n"
      "continuation line\n"
      "2026-05-03 01:02:04:005 E second line\n"
  );

  std::string command =
    ToolCommand(LOGMEOBF_EXE)
    + " --obfuscate --key-base64 "
    + QuoteText(FIXED_KEY_BASE64)
    + " --nonce-salt 0 --format text --in "
    + Quote(input)
    + " --out "
    + Quote(obf);

  ASSERT_EQ(0, Run(command));

  EXPECT_EQ(
    "5AA537000000000000000000000000002A88700780D693FC23527C519E797E15C281C0CFD19D6F7CA5D89A7B5846017C1E616608DB8D5333E9E83651057A6953F5809A37B038535AA52600000000000100000000000000AD67C169981A9A3519858E6C63AFA9B5FF4DDF34775B558D8644C63EEE87B3C0F826BFC281C2"
    , Hex(ReadBytes(obf))
  );

  command =
    ToolCommand(LOGMEOBF_EXE)
    + " --deobfuscate --key-base64 "
    + QuoteText(FIXED_KEY_BASE64)
    + " --in "
    + Quote(obf)
    + " --out "
    + Quote(plain);

  ASSERT_EQ(0, Run(command));
  EXPECT_EQ(ReadText(input), ReadText(plain));
}

TEST(LogmeObfTool, ObfuscatesJsonAndDeobfuscatesBack)
{
  fs::path dir = MakeTempDir("obf_json_roundtrip");
  fs::path input = dir / "input.jsonl";
  fs::path obf = dir / "output.obf";
  fs::path plain = dir / "plain.jsonl";

  WriteText(
    input
    , "{\"level\":\"INFO\",\"message\":\"hello\"}\n"
      "{\"level\":\"WARN\",\"message\":\"world\"}\n"
  );

  std::string command =
    ToolCommand(LOGMEOBF_EXE)
    + " --obfuscate --key-base64 "
    + QuoteText(FIXED_KEY_BASE64)
    + " --nonce-salt 0 --format auto --in "
    + Quote(input)
    + " --out "
    + Quote(obf);

  ASSERT_EQ(0, Run(command));

  command =
    ToolCommand(LOGMEOBF_EXE)
    + " --deobfuscate --key-base64 "
    + QuoteText(FIXED_KEY_BASE64)
    + " --in "
    + Quote(obf)
    + " --out "
    + Quote(plain);

  ASSERT_EQ(0, Run(command));
  EXPECT_EQ(ReadText(input), ReadText(plain));
}
