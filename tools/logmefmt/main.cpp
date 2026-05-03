#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace
{
  enum class Format
  {
    Text,
    Json,
    Xml
  };

  struct Record
  {
    std::vector<std::pair<std::string, std::string>> Fields;
  };

  struct Options
  {
    Format Input = Format::Text;
    Format Output = Format::Text;
    bool Finalize = false;
    bool Help = false;
    std::string Root = "log";
    std::string InputPath;
    std::string OutputPath;
    std::map<std::string, std::string> InputFields;
    std::map<std::string, std::string> OutputFields;
  };

  static void PrintUsage()
  {
    std::cout
      << "Usage: logmefmt --input text|json|xml --output text|json|xml [options]\n"
      << "\n"
      << "Options:\n"
      << "  --input FORMAT          Input format: text, json, xml\n"
      << "  --output FORMAT         Output format: text, json, xml\n"
      << "  --in FILE               Read from FILE instead of stdin\n"
      << "  --out FILE              Write to FILE instead of stdout\n"
      << "  --finalize              Complete JSON/XML output document\n"
      << "  --root NAME             XML root element for --finalize (default: log)\n"
      << "  --field OLD=NEW         Rename field on both input and output sides\n"
      << "  --input-field OLD=NEW   Rename field before conversion\n"
      << "  --output-field OLD=NEW  Rename field after conversion\n"
      << "  --help                  Show this help\n"
    ;
  }

  static bool ParseFormat(const std::string& text, Format& format)
  {
    if (text == "text")
    {
      format = Format::Text;
      return true;
    }

    if (text == "json" || text == "jsonl")
    {
      format = Format::Json;
      return true;
    }

    if (text == "xml")
    {
      format = Format::Xml;
      return true;
    }

    return false;
  }

  static bool SplitMapping(const std::string& text, std::string& from, std::string& to)
  {
    size_t pos = text.find('=');
    if (pos == std::string::npos || pos == 0 || pos + 1 >= text.size())
      return false;

    from = text.substr(0, pos);
    to = text.substr(pos + 1);
    return true;
  }

  static bool ReadNextArg(int& index, int argc, char** argv, std::string& value)
  {
    if (index + 1 >= argc)
      return false;

    ++index;
    value = argv[index];
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

      if (arg == "--finalize")
      {
        options.Finalize = true;
        continue;
      }

      if (arg == "--input" || arg == "--output" || arg == "--root" || arg == "--field"
        || arg == "--input-field" || arg == "--output-field" || arg == "--in" || arg == "--out")
      {
        if (!ReadNextArg(i, argc, argv, value))
        {
          std::cerr << "Missing value for " << arg << "\n";
          return false;
        }
      }
      else
      {
        std::cerr << "Unknown option: " << arg << "\n";
        return false;
      }

      if (arg == "--input")
      {
        if (!ParseFormat(value, options.Input))
        {
          std::cerr << "Unknown input format: " << value << "\n";
          return false;
        }
      }
      else if (arg == "--output")
      {
        if (!ParseFormat(value, options.Output))
        {
          std::cerr << "Unknown output format: " << value << "\n";
          return false;
        }
      }
      else if (arg == "--root")
      {
        options.Root = value;
      }
      else if (arg == "--in")
      {
        options.InputPath = value;
      }
      else if (arg == "--out")
      {
        options.OutputPath = value;
      }
      else
      {
        std::string from;
        std::string to;
        if (!SplitMapping(value, from, to))
        {
          std::cerr << "Invalid field mapping: " << value << "\n";
          return false;
        }

        if (arg == "--field" || arg == "--input-field")
          options.InputFields[from] = to;

        if (arg == "--field" || arg == "--output-field")
          options.OutputFields[from] = to;
      }
    }

    return true;
  }

  static std::string Trim(const std::string& text)
  {
    size_t begin = 0;
    while (begin < text.size() && std::isspace((unsigned char)text[begin]))
      ++begin;

    size_t end = text.size();
    while (end > begin && std::isspace((unsigned char)text[end - 1]))
      --end;

    return text.substr(begin, end - begin);
  }

  static void RenameFields(Record& record, const std::map<std::string, std::string>& fields)
  {
    if (fields.empty())
      return;

    for (auto& item : record.Fields)
    {
      auto it = fields.find(item.first);
      if (it != fields.end())
        item.first = it->second;
    }
  }

  static void AppendJsonEscaped(std::string& output, const std::string& value)
  {
    output.push_back('"');

    for (unsigned char c : value)
    {
      switch (c)
      {
      case '"': output += "\\\""; break;
      case '\\': output += "\\\\"; break;
      case '\b': output += "\\b"; break;
      case '\f': output += "\\f"; break;
      case '\n': output += "\\n"; break;
      case '\r': output += "\\r"; break;
      case '\t': output += "\\t"; break;
      default:
        if (c < 0x20)
        {
          static const char* hex = "0123456789ABCDEF";
          output += "\\u00";
          output.push_back(hex[(c >> 4) & 0x0F]);
          output.push_back(hex[c & 0x0F]);
        }
        else
        {
          output.push_back((char)c);
        }
      }
    }

    output.push_back('"');
  }

  static void AppendXmlEscaped(std::string& output, const std::string& value)
  {
    for (char c : value)
    {
      switch (c)
      {
      case '&': output += "&amp;"; break;
      case '<': output += "&lt;"; break;
      case '>': output += "&gt;"; break;
      case '"': output += "&quot;"; break;
      case '\'': output += "&apos;"; break;
      default: output.push_back(c); break;
      }
    }
  }

  static bool ReadJsonString(const std::string& text, size_t& pos, std::string& value)
  {
    if (pos >= text.size() || text[pos] != '"')
      return false;

    ++pos;
    value.clear();

    while (pos < text.size())
    {
      char c = text[pos++];
      if (c == '"')
        return true;

      if (c != '\\')
      {
        value.push_back(c);
        continue;
      }

      if (pos >= text.size())
        return false;

      c = text[pos++];
      switch (c)
      {
      case '"': value.push_back('"'); break;
      case '\\': value.push_back('\\'); break;
      case '/': value.push_back('/'); break;
      case 'b': value.push_back('\b'); break;
      case 'f': value.push_back('\f'); break;
      case 'n': value.push_back('\n'); break;
      case 'r': value.push_back('\r'); break;
      case 't': value.push_back('\t'); break;
      case 'u':
        if (pos + 4 > text.size())
          return false;

        value += "\\u";
        value.append(text, pos, 4);
        pos += 4;
        break;

      default:
        value.push_back(c);
        break;
      }
    }

    return false;
  }

  static void SkipSpaces(const std::string& text, size_t& pos)
  {
    while (pos < text.size() && std::isspace((unsigned char)text[pos]))
      ++pos;
  }

  static bool ReadJsonRawValue(const std::string& text, size_t& pos, std::string& value)
  {
    size_t begin = pos;
    while (pos < text.size() && text[pos] != ',' && text[pos] != '}')
      ++pos;

    value = Trim(text.substr(begin, pos - begin));
    return !value.empty();
  }

  static bool ParseJsonRecord(const std::string& line, Record& record)
  {
    std::string text = Trim(line);
    record.Fields.clear();

    if (text.empty())
      return false;

    size_t pos = 0;
    SkipSpaces(text, pos);
    if (pos >= text.size() || text[pos] != '{')
      return false;

    ++pos;

    while (pos < text.size())
    {
      SkipSpaces(text, pos);
      if (pos < text.size() && text[pos] == '}')
        return true;

      std::string name;
      if (!ReadJsonString(text, pos, name))
        return false;

      SkipSpaces(text, pos);
      if (pos >= text.size() || text[pos] != ':')
        return false;

      ++pos;
      SkipSpaces(text, pos);

      std::string value;
      if (pos < text.size() && text[pos] == '"')
      {
        if (!ReadJsonString(text, pos, value))
          return false;
      }
      else if (!ReadJsonRawValue(text, pos, value))
      {
        return false;
      }

      record.Fields.push_back({name, value});

      SkipSpaces(text, pos);
      if (pos < text.size() && text[pos] == ',')
      {
        ++pos;
        continue;
      }

      if (pos < text.size() && text[pos] == '}')
        return true;
    }

    return false;
  }

  static void ReplaceAll(std::string& text, const std::string& from, const std::string& to)
  {
    size_t pos = 0;
    while ((pos = text.find(from, pos)) != std::string::npos)
    {
      text.replace(pos, from.size(), to);
      pos += to.size();
    }
  }

  static std::string XmlUnescape(std::string value)
  {
    ReplaceAll(value, "&quot;", "\"");
    ReplaceAll(value, "&apos;", "'");
    ReplaceAll(value, "&lt;", "<");
    ReplaceAll(value, "&gt;", ">");
    ReplaceAll(value, "&amp;", "&");
    return value;
  }

  static bool ParseXmlRecord(const std::string& line, Record& record)
  {
    std::string text = Trim(line);
    record.Fields.clear();

    if (text.empty())
      return false;

    size_t eventBegin = text.find('>');
    size_t eventEnd = text.rfind("</");
    if (eventBegin == std::string::npos || eventEnd == std::string::npos || eventBegin >= eventEnd)
      return false;

    size_t pos = eventBegin + 1;
    while (pos < eventEnd)
    {
      size_t open = text.find('<', pos);
      if (open == std::string::npos || open >= eventEnd)
        break;

      if (open + 1 < text.size() && text[open + 1] == '/')
        break;

      size_t close = text.find('>', open + 1);
      if (close == std::string::npos)
        return false;

      std::string name = text.substr(open + 1, close - open - 1);
      size_t space = name.find_first_of(" \t\r\n");
      if (space != std::string::npos)
        name.resize(space);

      std::string closeTag = "</" + name + ">";
      size_t valueEnd = text.find(closeTag, close + 1);
      if (valueEnd == std::string::npos)
        return false;

      std::string value = text.substr(close + 1, valueEnd - close - 1);
      record.Fields.push_back({name, XmlUnescape(value)});
      pos = valueEnd + closeTag.size();
    }

    return true;
  }

  static bool ParseTextRecord(const std::string& line, Record& record)
  {
    record.Fields.clear();
    record.Fields.push_back({"message", line});
    return true;
  }

  static bool ParseRecord(const std::string& line, Format format, Record& record)
  {
    switch (format)
    {
    case Format::Json: return ParseJsonRecord(line, record);
    case Format::Xml: return ParseXmlRecord(line, record);
    case Format::Text: return ParseTextRecord(line, record);
    default: return false;
    }
  }

  static std::string FormatJsonRecord(const Record& record)
  {
    std::string output;
    output.reserve(128);
    output.push_back('{');

    for (size_t i = 0; i < record.Fields.size(); ++i)
    {
      if (i)
        output.push_back(',');

      AppendJsonEscaped(output, record.Fields[i].first);
      output.push_back(':');
      AppendJsonEscaped(output, record.Fields[i].second);
    }

    output.push_back('}');
    return output;
  }

  static std::string FormatXmlRecord(const Record& record)
  {
    std::string output;
    output.reserve(128);
    output += "<event>";

    for (const auto& field : record.Fields)
    {
      output.push_back('<');
      output += field.first;
      output.push_back('>');
      AppendXmlEscaped(output, field.second);
      output += "</";
      output += field.first;
      output.push_back('>');
    }

    output += "</event>";
    return output;
  }

  static std::string FormatTextRecord(const Record& record)
  {
    for (const auto& field : record.Fields)
    {
      if (field.first == "message" || field.first == "msg")
        return field.second;
    }

    std::string output;
    for (size_t i = 0; i < record.Fields.size(); ++i)
    {
      if (i)
        output.push_back(' ');

      output += record.Fields[i].first;
      output.push_back('=');
      output += record.Fields[i].second;
    }

    return output;
  }

  static std::string FormatRecord(const Record& record, Format format)
  {
    switch (format)
    {
    case Format::Json: return FormatJsonRecord(record);
    case Format::Xml: return FormatXmlRecord(record);
    case Format::Text: return FormatTextRecord(record);
    default: return std::string();
    }
  }

  static void WriteBegin(std::ostream& output, const Options& options)
  {
    if (!options.Finalize)
      return;

    if (options.Output == Format::Json)
      output << "[\n";
    else if (options.Output == Format::Xml)
      output << "<" << options.Root << ">\n";
  }

  static void WriteEnd(std::ostream& output, const Options& options, bool empty)
  {
    if (!options.Finalize)
      return;

    if (options.Output == Format::Json)
    {
      if (!empty)
        output << "\n";

      output << "]\n";
    }
    else if (options.Output == Format::Xml)
    {
      if (empty)
        output << "</" << options.Root << ">\n";
      else
        output << "</" << options.Root << ">\n";
    }
  }

  static void WriteRecord(std::ostream& output, const Options& options, const Record& record, bool first)
  {
    std::string formatted = FormatRecord(record, options.Output);

    if (options.Finalize && options.Output == Format::Json)
    {
      if (!first)
        output << ",\n";

      output << "  " << formatted;
      return;
    }

    if (options.Finalize && options.Output == Format::Xml)
    {
      output << "  " << formatted << "\n";
      return;
    }

    output << formatted << "\n";
  }

  static bool Convert(std::istream& input, std::ostream& output, const Options& options)
  {
    WriteBegin(output, options);

    std::string line;
    Record record;
    bool first = true;

    while (std::getline(input, line))
    {
      if (Trim(line).empty())
        continue;

      if (!ParseRecord(line, options.Input, record))
      {
        std::cerr << "Failed to parse input record: " << line << "\n";
        return false;
      }

      RenameFields(record, options.InputFields);
      RenameFields(record, options.OutputFields);
      WriteRecord(output, options, record, first);
      first = false;
    }

    WriteEnd(output, options, first);
    return true;
  }
}

int main(int argc, char** argv)
{
  Options options;
  if (!ParseOptions(argc, argv, options))
    return 1;

  if (options.Help)
  {
    PrintUsage();
    return 0;
  }

  std::ifstream inputFile;
  std::ofstream outputFile;

  std::istream* input = &std::cin;
  std::ostream* output = &std::cout;

  if (!options.InputPath.empty())
  {
    inputFile.open(options.InputPath, std::ios::binary);
    if (!inputFile)
    {
      std::cerr << "Cannot open input file: " << options.InputPath << "\n";
      return 2;
    }

    input = &inputFile;
  }

  if (!options.OutputPath.empty())
  {
    outputFile.open(options.OutputPath, std::ios::binary);
    if (!outputFile)
    {
      std::cerr << "Cannot open output file: " << options.OutputPath << "\n";
      return 2;
    }

    output = &outputFile;
  }

  return Convert(*input, *output, options) ? 0 : 3;
}
