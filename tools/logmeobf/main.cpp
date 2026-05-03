#include <Logme/Obfuscate.h>

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace
{

  static void StoreLe16(uint8_t* p, uint16_t v)
  {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
  }

  static void StoreLe32(uint8_t* p, uint32_t v)
  {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
  }

  static uint32_t LoadLe32(const uint8_t* p)
  {
    return
      ((uint32_t)p[0])
      | ((uint32_t)p[1] << 8)
      | ((uint32_t)p[2] << 16)
      | ((uint32_t)p[3] << 24);
  }

  static void StoreLe64(uint8_t* p, uint64_t v)
  {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
    p[4] = (uint8_t)(v >> 32);
    p[5] = (uint8_t)(v >> 40);
    p[6] = (uint8_t)(v >> 48);
    p[7] = (uint8_t)(v >> 56);
  }

  static void LocalNonceGenInit(NonceGen* gen)
  {
    if (gen == nullptr)
      return;

    gen->Counter = 0;
    gen->Salt =
      (uint32_t)time(nullptr)
      ^ (uint32_t)(uintptr_t)gen;
  }

  static void LocalNonceGenNext(
    NonceGen* gen
    , uint8_t nonce[LOGOBF_NONCE_BYTES]
  )
  {
    if (gen == nullptr || nonce == nullptr)
      return;

    uint64_t counter = gen->Counter++;
    StoreLe32(nonce + 0, gen->Salt);
    StoreLe64(nonce + 4, counter);
  }

  static uint32_t Rotl32(uint32_t v, int r)
  {
    return (v << r) | (v >> (32 - r));
  }

  static void QuarterRound(
    uint32_t* a
    , uint32_t* b
    , uint32_t* c
    , uint32_t* d
  )
  {
    *a += *b;
    *d ^= *a;
    *d = Rotl32(*d, 16);

    *c += *d;
    *b ^= *c;
    *b = Rotl32(*b, 12);

    *a += *b;
    *d ^= *a;
    *d = Rotl32(*d, 8);

    *c += *d;
    *b ^= *c;
    *b = Rotl32(*b, 7);
  }

  static void ChaCha20Block(
    const uint8_t key[32]
    , uint32_t counter
    , const uint8_t nonce[12]
    , uint8_t out[64]
  )
  {
    uint32_t state[16]{};
    uint32_t work[16]{};

    state[0] = 0x61707865u;
    state[1] = 0x3320646eu;
    state[2] = 0x79622d32u;
    state[3] = 0x6b206574u;

    state[4] = LoadLe32(key + 0);
    state[5] = LoadLe32(key + 4);
    state[6] = LoadLe32(key + 8);
    state[7] = LoadLe32(key + 12);
    state[8] = LoadLe32(key + 16);
    state[9] = LoadLe32(key + 20);
    state[10] = LoadLe32(key + 24);
    state[11] = LoadLe32(key + 28);

    state[12] = counter;
    state[13] = LoadLe32(nonce + 0);
    state[14] = LoadLe32(nonce + 4);
    state[15] = LoadLe32(nonce + 8);

    memcpy(work, state, sizeof(state));

    for (int i = 0; i < 10; ++i)
    {
      QuarterRound(&work[0], &work[4], &work[8], &work[12]);
      QuarterRound(&work[1], &work[5], &work[9], &work[13]);
      QuarterRound(&work[2], &work[6], &work[10], &work[14]);
      QuarterRound(&work[3], &work[7], &work[11], &work[15]);

      QuarterRound(&work[0], &work[5], &work[10], &work[15]);
      QuarterRound(&work[1], &work[6], &work[11], &work[12]);
      QuarterRound(&work[2], &work[7], &work[8], &work[13]);
      QuarterRound(&work[3], &work[4], &work[9], &work[14]);
    }

    for (int i = 0; i < 16; ++i)
      StoreLe32(out + (size_t)i * 4, work[i] + state[i]);
  }

  static void ChaCha20Xor(
    const uint8_t key[32]
    , const uint8_t nonce[12]
    , uint32_t counter
    , const uint8_t* input
    , uint8_t* output
    , size_t len
  )
  {
    uint8_t block[64];

    while (len != 0)
    {
      ChaCha20Block(key, counter++, nonce, block);

      size_t count = (len > 64) ? 64 : len;
      for (size_t i = 0; i < count; ++i)
        output[i] = (uint8_t)(input[i] ^ block[i]);

      input += count;
      output += count;
      len -= count;
    }

    memset(block, 0, sizeof(block));
  }

  static size_t LocalObfCalcRecordSize(size_t plaintextLen)
  {
    if (plaintextLen > 0xffffu)
      return 0;

    return LOGOBF_HEADER_BYTES + plaintextLen;
  }

  static bool LocalObfEncryptRecord(
    const ObfKey* key
    , NonceGen* nonceGen
    , const uint8_t* plaintext
    , size_t plaintextLen
    , uint8_t* outRecord
    , size_t outRecordCap
    , size_t* outRecordLen
  )
  {
    if (key == nullptr || nonceGen == nullptr || outRecord == nullptr || outRecordLen == nullptr)
      return false;

    size_t required = LocalObfCalcRecordSize(plaintextLen);
    if (required == 0 || outRecordCap < required)
      return false;

    uint8_t* header = outRecord;
    uint8_t* nonce = header + 4;
    uint8_t* ciphertext = header + LOGOBF_HEADER_BYTES;

    StoreLe16(header + 0, LOGOBF_SIGNATURE);
    StoreLe16(header + 2, (uint16_t)plaintextLen);
    LocalNonceGenNext(nonceGen, nonce);

    ChaCha20Xor(
      key->Bytes
      , nonce
      , 1
      , plaintext
      , ciphertext
      , plaintextLen
    );

    *outRecordLen = required;
    return true;
  }

  static bool LocalObfDecryptRecord(
    const ObfKey* key
    , const uint8_t* record
    , size_t recordLen
    , uint8_t* outPlaintext
    , size_t outPlaintextCap
    , size_t* outPlaintextLen
  )
  {
    if (key == nullptr || record == nullptr || outPlaintext == nullptr || outPlaintextLen == nullptr)
      return false;

    if (recordLen < LOGOBF_HEADER_BYTES)
      return false;

    uint16_t signature = (uint16_t)((uint16_t)record[0] | ((uint16_t)record[1] << 8));
    uint16_t len = (uint16_t)((uint16_t)record[2] | ((uint16_t)record[3] << 8));

    if (signature != LOGOBF_SIGNATURE)
      return false;

    size_t required = LOGOBF_HEADER_BYTES + (size_t)len;
    if (recordLen < required || outPlaintextCap < (size_t)len)
      return false;

    ChaCha20Xor(
      key->Bytes
      , record + 4
      , 1
      , record + LOGOBF_HEADER_BYTES
      , outPlaintext
      , (size_t)len
    );

    *outPlaintextLen = (size_t)len;
    return true;
  }

  enum class Mode
  {
    None,
    GenerateKey,
    Obfuscate,
    Deobfuscate
  };

  enum class TextFormat
  {
    Auto,
    Text,
    Json,
    Xml
  };

  struct Options
  {
    Mode Action = Mode::None;
    TextFormat Format = TextFormat::Auto;
    bool PrintBase64 = false;
    bool Help = false;
    bool HasNonceSalt = false;
    uint32_t NonceSalt = 0;
    std::string KeyFile;
    std::string KeyBase64;
    std::string KeyOut;
    std::string HeaderOut;
    std::string HeaderName = "LOGME_OBFUSCATION_KEY";
    std::string InputPath;
    std::string OutputPath;
  };

  static void PrintUsage()
  {
    std::cout
      << "Usage:\n"
      << "  logmeobf --generate-key [--key-out FILE] [--base64] [--header FILE] [--name IDENTIFIER]\n"
      << "  logmeobf --obfuscate --key-file FILE|--key-base64 TEXT [--format auto|text|json|xml] [--in FILE] [--out FILE]\n"
      << "  logmeobf --deobfuscate --key-file FILE|--key-base64 TEXT [--in FILE] [--out FILE]\n"
      << "\n"
      << "Options:\n"
      << "  --generate-key          Generate a new 32-byte obfuscation key\n"
      << "  --obfuscate             Convert readable log records to binary obfuscated records\n"
      << "  --deobfuscate           Convert binary obfuscated records to readable log records\n"
      << "  --key-file FILE         Read binary key from FILE\n"
      << "  --key-base64 TEXT       Read base64 key from TEXT\n"
      << "  --key-out FILE          Write generated key as binary file\n"
      << "  --base64                Print generated key as base64\n"
      << "  --header FILE           Write generated key as C/C++ header\n"
      << "  --name IDENTIFIER       Header variable name\n"
      << "  --in FILE               Read input from FILE instead of stdin\n"
      << "  --out FILE              Write output to FILE instead of stdout\n"
      << "  --format FORMAT         Input readable format for --obfuscate: auto, text, json, xml\n"
      << "  --nonce-salt VALUE      Use fixed nonce salt for reproducible obfuscation output\n"
      << "  --help                  Show this help\n"
    ;
  }

  static bool ReadNextArg(int& index, int argc, char** argv, std::string& value)
  {
    if (index + 1 >= argc)
      return false;

    ++index;
    value = argv[index];
    return true;
  }

  static bool ParseFormat(const std::string& value, TextFormat& format)
  {
    if (value == "auto")
    {
      format = TextFormat::Auto;
      return true;
    }

    if (value == "text")
    {
      format = TextFormat::Text;
      return true;
    }

    if (value == "json" || value == "jsonl")
    {
      format = TextFormat::Json;
      return true;
    }

    if (value == "xml")
    {
      format = TextFormat::Xml;
      return true;
    }

    return false;
  }

  static bool ParseUInt32(const std::string& text, uint32_t& value)
  {
    if (text.empty())
      return false;

    char* end = nullptr;
    unsigned long parsed = std::strtoul(text.c_str(), &end, 0);
    if (end == nullptr || *end != 0 || parsed > 0xfffffffful)
      return false;

    value = (uint32_t)parsed;
    return true;
  }

  static bool ParseOptions(int argc, char** argv, Options& options)
  {
    for (int i = 1; i < argc; ++i)
    {
      std::string arg = argv[i];
      std::string value;

      if (arg == "--help" || arg == "-h")
      {
        options.Help = true;
        return true;
      }

      if (arg == "--generate-key")
      {
        options.Action = Mode::GenerateKey;
        continue;
      }

      if (arg == "--obfuscate")
      {
        options.Action = Mode::Obfuscate;
        continue;
      }

      if (arg == "--deobfuscate")
      {
        options.Action = Mode::Deobfuscate;
        continue;
      }

      if (arg == "--base64")
      {
        options.PrintBase64 = true;
        continue;
      }

      if (arg == "--key-file" || arg == "--key-base64" || arg == "--key-out" || arg == "--header"
        || arg == "--name" || arg == "--in" || arg == "--out" || arg == "--format" || arg == "--nonce-salt")
      {
        if (!ReadNextArg(i, argc, argv, value))
        {
          std::cerr << "Missing value for " << arg << "\n";
          return false;
        }

        if (arg == "--key-file")
          options.KeyFile = value;
        else if (arg == "--key-base64")
          options.KeyBase64 = value;
        else if (arg == "--key-out")
          options.KeyOut = value;
        else if (arg == "--header")
          options.HeaderOut = value;
        else if (arg == "--name")
          options.HeaderName = value;
        else if (arg == "--in")
          options.InputPath = value;
        else if (arg == "--out")
          options.OutputPath = value;
        else if (arg == "--nonce-salt")
        {
          if (!ParseUInt32(value, options.NonceSalt))
          {
            std::cerr << "Invalid nonce salt: " << value << "\n";
            return false;
          }

          options.HasNonceSalt = true;
        }
        else if (!ParseFormat(value, options.Format))
        {
          std::cerr << "Invalid format: " << value << "\n";
          return false;
        }

        continue;
      }

      std::cerr << "Unknown option: " << arg << "\n";
      return false;
    }

    return true;
  }

  static const char Base64Chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  static std::string Base64Encode(const uint8_t* data, size_t size)
  {
    std::string out;
    out.reserve(((size + 2) / 3) * 4);

    for (size_t i = 0; i < size; i += 3)
    {
      uint32_t v = (uint32_t)data[i] << 16;
      if (i + 1 < size)
        v |= (uint32_t)data[i + 1] << 8;
      if (i + 2 < size)
        v |= (uint32_t)data[i + 2];

      out.push_back(Base64Chars[(v >> 18) & 63]);
      out.push_back(Base64Chars[(v >> 12) & 63]);
      out.push_back((i + 1 < size) ? Base64Chars[(v >> 6) & 63] : '=');
      out.push_back((i + 2 < size) ? Base64Chars[v & 63] : '=');
    }

    return out;
  }

  static int Base64Value(char c)
  {
    if (c >= 'A' && c <= 'Z')
      return c - 'A';
    if (c >= 'a' && c <= 'z')
      return c - 'a' + 26;
    if (c >= '0' && c <= '9')
      return c - '0' + 52;
    if (c == '+')
      return 62;
    if (c == '/')
      return 63;
    return -1;
  }

  static bool Base64Decode(const std::string& text, std::vector<uint8_t>& out)
  {
    int values[4];
    int count = 0;
    out.clear();

    for (char ch : text)
    {
      if (std::isspace((unsigned char)ch))
        continue;

      if (ch == '=')
        values[count++] = -2;
      else
      {
        int value = Base64Value(ch);
        if (value < 0)
          return false;

        values[count++] = value;
      }

      if (count != 4)
        continue;

      if (values[0] < 0 || values[1] < 0)
        return false;

      uint32_t v = ((uint32_t)values[0] << 18) | ((uint32_t)values[1] << 12);
      if (values[2] >= 0)
        v |= (uint32_t)values[2] << 6;
      if (values[3] >= 0)
        v |= (uint32_t)values[3];

      out.push_back((uint8_t)((v >> 16) & 0xff));
      if (values[2] >= 0)
        out.push_back((uint8_t)((v >> 8) & 0xff));
      if (values[3] >= 0)
        out.push_back((uint8_t)(v & 0xff));

      count = 0;
    }

    return count == 0;
  }

  static bool ReadFileBinary(const std::string& path, std::vector<uint8_t>& data)
  {
    std::ifstream in(path, std::ios::binary);
    if (!in)
      return false;

    data.assign(
      std::istreambuf_iterator<char>(in)
      , std::istreambuf_iterator<char>()
    );
    return true;
  }

  static bool WriteFileBinary(const std::string& path, const uint8_t* data, size_t size)
  {
    std::ofstream out(path, std::ios::binary);
    if (!out)
      return false;

    out.write((const char*)data, (std::streamsize)size);
    return (bool)out;
  }

  static bool LoadKey(const Options& options, ObfKey& key)
  {
    std::vector<uint8_t> data;

    if (!options.KeyFile.empty())
    {
      if (!ReadFileBinary(options.KeyFile, data))
      {
        std::cerr << "Failed to read key file: " << options.KeyFile << "\n";
        return false;
      }
    }
    else if (!options.KeyBase64.empty())
    {
      if (!Base64Decode(options.KeyBase64, data))
      {
        std::cerr << "Invalid base64 key\n";
        return false;
      }
    }
    else
    {
      std::cerr << "Key is required. Use --key-file or --key-base64.\n";
      return false;
    }

    if (data.size() != LOGOBF_KEY_BYTES)
    {
      std::cerr << "Invalid key size: " << data.size() << ", expected " << LOGOBF_KEY_BYTES << "\n";
      return false;
    }

    memcpy(key.Bytes, data.data(), LOGOBF_KEY_BYTES);
    return true;
  }

  static void GenerateKey(ObfKey& key)
  {
    std::random_device rd;
    for (size_t i = 0; i < LOGOBF_KEY_BYTES; ++i)
      key.Bytes[i] = (uint8_t)rd();
  }

  static bool WriteHeaderKey(const std::string& path, const std::string& name, const ObfKey& key)
  {
    std::ofstream out(path, std::ios::binary);
    if (!out)
      return false;

    out << "#pragma once\n\n";
    out << "#include <Logme/Obfuscate.h>\n\n";
    out << "static const ObfKey " << name << " = {{\n  ";

    for (size_t i = 0; i < LOGOBF_KEY_BYTES; ++i)
    {
      if (i != 0)
      {
        out << ", ";
        if (i % 8 == 0)
          out << "\n  ";
      }

      char buffer[8];
      snprintf(buffer, sizeof(buffer), "0x%02X", (unsigned int)key.Bytes[i]);
      out << buffer;
    }

    out << "\n}};\n";
    return (bool)out;
  }

  static std::string ReadAllStdin()
  {
    return std::string(
      std::istreambuf_iterator<char>(std::cin)
      , std::istreambuf_iterator<char>()
    );
  }

  static bool ReadInputText(const Options& options, std::string& text)
  {
    if (options.InputPath.empty())
    {
      text = ReadAllStdin();
      return true;
    }

    std::vector<uint8_t> data;
    if (!ReadFileBinary(options.InputPath, data))
      return false;

    text.assign((const char*)data.data(), data.size());
    return true;
  }

  static bool WriteOutputBytes(const Options& options, const std::vector<uint8_t>& data)
  {
    if (options.OutputPath.empty())
    {
      std::cout.write((const char*)data.data(), (std::streamsize)data.size());
      return (bool)std::cout;
    }

    return WriteFileBinary(options.OutputPath, data.data(), data.size());
  }

  static std::vector<std::string> SplitLinesKeepEol(const std::string& text)
  {
    std::vector<std::string> lines;
    size_t pos = 0;

    while (pos < text.size())
    {
      size_t end = text.find('\n', pos);
      if (end == std::string::npos)
      {
        lines.push_back(text.substr(pos));
        break;
      }

      lines.push_back(text.substr(pos, end - pos + 1));
      pos = end + 1;
    }

    return lines;
  }

  static std::string TrimLeft(const std::string& text)
  {
    size_t pos = 0;
    while (pos < text.size() && std::isspace((unsigned char)text[pos]))
      ++pos;

    return text.substr(pos);
  }

  static bool StartsWith(const std::string& text, const char* prefix)
  {
    size_t len = strlen(prefix);
    return text.size() >= len && memcmp(text.data(), prefix, len) == 0;
  }

  static bool IsJsonRecordLine(const std::string& line)
  {
    std::string s = TrimLeft(line);
    return !s.empty() && s[0] == '{';
  }

  static bool IsXmlRecordLine(const std::string& line)
  {
    std::string s = TrimLeft(line);
    return StartsWith(s, "<event") || StartsWith(s, "<record");
  }

  static bool LooksLikeTimestamp(const std::string& line)
  {
    if (line.size() < 24)
      return false;

    return
      std::isdigit((unsigned char)line[0])
      && std::isdigit((unsigned char)line[1])
      && std::isdigit((unsigned char)line[2])
      && std::isdigit((unsigned char)line[3])
      && line[4] == '-'
      && std::isdigit((unsigned char)line[5])
      && std::isdigit((unsigned char)line[6])
      && line[7] == '-'
      && std::isdigit((unsigned char)line[8])
      && std::isdigit((unsigned char)line[9])
      && (line[10] == ' ' || line[10] == 'T')
      && std::isdigit((unsigned char)line[11])
      && std::isdigit((unsigned char)line[12])
      && line[13] == ':'
      && std::isdigit((unsigned char)line[14])
      && std::isdigit((unsigned char)line[15])
      && line[16] == ':'
      && std::isdigit((unsigned char)line[17])
      && std::isdigit((unsigned char)line[18])
      && line[19] == ':'
      && std::isdigit((unsigned char)line[20])
      && std::isdigit((unsigned char)line[21])
      && std::isdigit((unsigned char)line[22]);
  }

  static bool LooksLikeSignature(const std::string& line)
  {
    if (line.size() < 2)
      return false;

    char c = line[0];
    return (c == 'D' || c == 'I' || c == 'W' || c == 'E' || c == 'C') && line[1] == ' ';
  }

  static bool IsTextRecordStart(const std::string& line)
  {
    return LooksLikeTimestamp(line) || LooksLikeSignature(line);
  }

  static TextFormat DetectFormat(const std::vector<std::string>& lines)
  {
    int json = 0;
    int xml = 0;
    int text = 0;
    int seen = 0;

    for (const std::string& line : lines)
    {
      std::string trimmed = TrimLeft(line);
      if (trimmed.empty())
        continue;

      if (IsJsonRecordLine(line))
        ++json;
      else if (IsXmlRecordLine(line))
        ++xml;
      else if (IsTextRecordStart(line))
        ++text;

      ++seen;
      if (seen >= 10)
        break;
    }

    if (json > xml && json > 0)
      return TextFormat::Json;

    if (xml > json && xml > 0)
      return TextFormat::Xml;

    return TextFormat::Text;
  }

  static std::vector<std::string> BuildRecords(const std::string& text, TextFormat format)
  {
    std::vector<std::string> lines = SplitLinesKeepEol(text);
    std::vector<std::string> records;

    if (format == TextFormat::Auto)
      format = DetectFormat(lines);

    if (format == TextFormat::Json || format == TextFormat::Xml)
    {
      for (const std::string& line : lines)
      {
        if (!line.empty())
          records.push_back(line);
      }
      return records;
    }

    std::string current;
    bool canGroup = false;

    for (const std::string& line : lines)
    {
      bool start = IsTextRecordStart(line);
      if (start)
        canGroup = true;

      if (canGroup && start && !current.empty())
      {
        records.push_back(current);
        current.clear();
      }

      if (!canGroup && !line.empty())
      {
        records.push_back(line);
        continue;
      }

      current += line;
    }

    if (!current.empty())
      records.push_back(current);

    return records;
  }

  static bool AppendEncryptedRecord(
    const ObfKey& key
    , NonceGen& nonceGen
    , const std::string& record
    , std::vector<uint8_t>& output
  )
  {
    size_t required = LocalObfCalcRecordSize(record.size());
    if (required == 0)
    {
      std::cerr << "Record is too large for logme obfuscation format\n";
      return false;
    }

    size_t offset = output.size();
    output.resize(offset + required);

    size_t written = 0;
    if (!LocalObfEncryptRecord(
      &key
      , &nonceGen
      , (const uint8_t*)record.data()
      , record.size()
      , output.data() + offset
      , required
      , &written
    ))
    {
      std::cerr << "Failed to obfuscate record\n";
      return false;
    }

    output.resize(offset + written);
    return true;
  }

  static bool RunGenerateKey(const Options& options)
  {
    ObfKey key{};
    GenerateKey(key);

    if (!options.KeyOut.empty() && !WriteFileBinary(options.KeyOut, key.Bytes, LOGOBF_KEY_BYTES))
    {
      std::cerr << "Failed to write key file: " << options.KeyOut << "\n";
      return false;
    }

    if (!options.HeaderOut.empty() && !WriteHeaderKey(options.HeaderOut, options.HeaderName, key))
    {
      std::cerr << "Failed to write header file: " << options.HeaderOut << "\n";
      return false;
    }

    if (options.PrintBase64 || (options.KeyOut.empty() && options.HeaderOut.empty()))
      std::cout << Base64Encode(key.Bytes, LOGOBF_KEY_BYTES) << "\n";

    return true;
  }

  static bool RunObfuscate(const Options& options)
  {
    ObfKey key{};
    if (!LoadKey(options, key))
      return false;

    std::string text;
    if (!ReadInputText(options, text))
    {
      std::cerr << "Failed to read input\n";
      return false;
    }

    std::vector<std::string> records = BuildRecords(text, options.Format);
    std::vector<uint8_t> output;
    NonceGen nonceGen{};
    LocalNonceGenInit(&nonceGen);
    if (options.HasNonceSalt)
    {
      nonceGen.Counter = 0;
      nonceGen.Salt = options.NonceSalt;
    }

    for (const std::string& record : records)
    {
      if (!AppendEncryptedRecord(key, nonceGen, record, output))
        return false;
    }

    if (!WriteOutputBytes(options, output))
    {
      std::cerr << "Failed to write output\n";
      return false;
    }

    return true;
  }

  static uint16_t LoadLe16(const uint8_t* p)
  {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
  }

  static bool RunDeobfuscate(const Options& options)
  {
    ObfKey key{};
    if (!LoadKey(options, key))
      return false;

    std::vector<uint8_t> input;
    if (options.InputPath.empty())
    {
      std::string text = ReadAllStdin();
      input.assign(text.begin(), text.end());
    }
    else if (!ReadFileBinary(options.InputPath, input))
    {
      std::cerr << "Failed to read input: " << options.InputPath << "\n";
      return false;
    }

    std::vector<uint8_t> output;
    size_t pos = 0;

    while (pos < input.size())
    {
      if (input.size() - pos < LOGOBF_HEADER_BYTES)
      {
        std::cerr << "Truncated obfuscated record\n";
        return false;
      }

      uint16_t len = LoadLe16(input.data() + pos + 2);
      size_t recordSize = LOGOBF_HEADER_BYTES + (size_t)len;
      if (input.size() - pos < recordSize)
      {
        std::cerr << "Truncated obfuscated record\n";
        return false;
      }

      size_t offset = output.size();
      output.resize(offset + len);

      size_t written = 0;
      if (!LocalObfDecryptRecord(
        &key
        , input.data() + pos
        , recordSize
        , output.data() + offset
        , len
        , &written
      ))
      {
        std::cerr << "Failed to deobfuscate record at offset " << pos << "\n";
        return false;
      }

      output.resize(offset + written);
      pos += recordSize;
    }

    if (!WriteOutputBytes(options, output))
    {
      std::cerr << "Failed to write output\n";
      return false;
    }

    return true;
  }
}

int main(int argc, char** argv)
{
  Options options;
  if (!ParseOptions(argc, argv, options))
    return 2;

  if (options.Help)
  {
    PrintUsage();
    return 0;
  }

  switch (options.Action)
  {
  case Mode::GenerateKey:
    return RunGenerateKey(options) ? 0 : 1;

  case Mode::Obfuscate:
    return RunObfuscate(options) ? 0 : 1;

  case Mode::Deobfuscate:
    return RunDeobfuscate(options) ? 0 : 1;

  default:
    PrintUsage();
    return 2;
  }
}
