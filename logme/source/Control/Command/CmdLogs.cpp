#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include <Logme/Logger.h>

#include "../CommandRegistrar.h"

using namespace Logme;

namespace fs = std::filesystem;

COMMAND_DESCRIPTOR2("logs", Logger::CommandLogs);

namespace
{
  const uint64_t DEFAULT_TAIL_BYTES = 256ULL * 1024ULL;
  const uint64_t DEFAULT_READ_BYTES = 512ULL * 1024ULL;
  const uint64_t MAX_READ_BYTES = 4ULL * 1024ULL * 1024ULL;
  const uint64_t MAX_DOWNLOAD_BYTES = 256ULL * 1024ULL * 1024ULL;

  std::vector<std::string> GetLogExtensions()
  {
    std::vector<std::string> extensions = Instance->GetHomeDirectoryLogExtensions();

    if (extensions.empty())
    {
      extensions.push_back(".log");
      extensions.push_back(".nlb");
      extensions.push_back(".nlr");
      extensions.push_back(".b64");
      extensions.push_back(".dat");
      extensions.push_back(".csv");
    }

    for (std::string& extension : extensions)
    {
      if (extension.empty())
        continue;

      if (extension[0] != '.')
        extension.insert(extension.begin(), '.');

      std::transform(
        extension.begin()
        , extension.end()
        , extension.begin()
        , [](unsigned char ch){ return (char)std::tolower(ch); }
      );
    }

    std::sort(extensions.begin(), extensions.end());
    extensions.erase(std::unique(extensions.begin(), extensions.end()), extensions.end());

    return extensions;
  }

  std::string NormalizeExtension(std::string extension)
  {
    std::transform(
      extension.begin()
      , extension.end()
      , extension.begin()
      , [](unsigned char ch){ return (char)std::tolower(ch); }
    );

    return extension;
  }

  bool IsUnsignedNumber(const std::string& text)
  {
    if (text.empty())
      return false;

    for (char ch : text)
    {
      if (!std::isdigit((unsigned char)ch))
        return false;
    }

    return true;
  }

  std::string JoinWords(
    const Logme::StringArray& arr
    , size_t begin
    , size_t end
  )
  {
    std::string text;

    for (size_t i = begin; i < end && i < arr.size(); ++i)
    {
      if (!text.empty())
        text += " ";

      text += arr[i];
    }

    return text;
  }

  bool IsAllowedLogFile(
    const fs::path& path
    , const std::unordered_set<std::string>& extensions
  )
  {
    std::string extension = NormalizeExtension(path.extension().string());
    return extensions.find(extension) != extensions.end();
  }

  bool IsPathInside(
    const fs::path& base
    , const fs::path& path
  )
  {
    auto baseIt = base.begin();
    auto pathIt = path.begin();

    for (; baseIt != base.end(); ++baseIt, ++pathIt)
    {
      if (pathIt == path.end())
        return false;

      if (*baseIt != *pathIt)
        return false;
    }

    return true;
  }

  bool ResolvePath(
    const std::string& relativePath
    , fs::path& fullPath
    , std::string& error
  )
  {
    std::error_code ec;
    fs::path home = fs::weakly_canonical(Instance->GetHomeDirectory(), ec);
    if (ec)
    {
      error = "unable to resolve home directory";
      return false;
    }

    fs::path requested = relativePath.empty()
      ? home
      : home / fs::path(relativePath);

    fs::path normalized = fs::weakly_canonical(requested, ec);
    if (ec)
    {
      error = "unable to resolve path";
      return false;
    }

    if (!IsPathInside(home, normalized))
    {
      error = "path is outside home directory";
      return false;
    }

    fullPath = normalized;
    return true;
  }

  std::string MakeRelative(const fs::path& path)
  {
    std::error_code ec;
    fs::path home = fs::weakly_canonical(Instance->GetHomeDirectory(), ec);
    if (ec)
      return path.string();

    fs::path relative = fs::relative(path, home, ec);
    if (ec)
      return path.string();

    std::string value = relative.generic_string();
    if (value == ".")
      return "";

    return value;
  }

  uint64_t GetFileSize(const fs::path& path)
  {
    std::error_code ec;
    uintmax_t size = fs::file_size(path, ec);
    if (ec)
      return 0;

    return (uint64_t)size;
  }

  uint64_t GetFileTime(const fs::path& path)
  {
    std::error_code ec;
    auto time = fs::last_write_time(path, ec);
    if (ec)
      return 0;

    return (uint64_t)time.time_since_epoch().count();
  }

  void AppendExtensions(std::string& response)
  {
    std::vector<std::string> extensions = GetLogExtensions();

    response += "Home directory: ";
    response += Instance->GetHomeDirectory();
    response += "\n";

    response += "Extensions:";
    for (const std::string& extension : extensions)
    {
      response += " ";
      response += extension;
    }
    response += "\n";
  }

  bool CommandInfo(std::string& response)
  {
    AppendExtensions(response);
    return true;
  }

  bool CommandTree(
    Logme::StringArray& arr
    , std::string& response
  )
  {
    std::string relativePath;
    if (arr.size() > 2)
      relativePath = JoinWords(arr, 2, arr.size());

    fs::path root;
    std::string error;
    if (!ResolvePath(relativePath, root, error))
    {
      response += "error: ";
      response += error;
      return false;
    }

    std::error_code ec;
    if (!fs::exists(root, ec))
    {
      response += "error: path does not exist";
      return false;
    }

    if (!fs::is_directory(root, ec))
    {
      response += "error: path is not a directory";
      return false;
    }

    std::vector<std::string> extensions = GetLogExtensions();
    std::unordered_set<std::string> extensionSet(extensions.begin(), extensions.end());
    std::vector<std::string> dirs;
    std::vector<std::string> files;

    for (const auto& entry : fs::directory_iterator(root, fs::directory_options::skip_permission_denied, ec))
    {
      if (ec)
        break;

      std::error_code statusEc;
      if (entry.is_directory(statusEc))
      {
        dirs.push_back(MakeRelative(entry.path()));
        continue;
      }

      if (!entry.is_regular_file(statusEc))
        continue;

      if (!IsAllowedLogFile(entry.path(), extensionSet))
        continue;

      std::ostringstream line;
      line << MakeRelative(entry.path())
        << "\t" << GetFileSize(entry.path())
        << "\t" << GetFileTime(entry.path());
      files.push_back(line.str());
    }

    std::sort(dirs.begin(), dirs.end());
    std::sort(files.begin(), files.end());

    response += "Home directory: ";
    response += Instance->GetHomeDirectory();
    response += "\n";
    response += "Path: ";
    response += relativePath;
    response += "\n";

    for (const std::string& dir : dirs)
    {
      response += "DIR\t";
      response += dir;
      response += "\n";
    }

    for (const std::string& file : files)
    {
      response += "FILE\t";
      response += file;
      response += "\n";
    }

    return true;
  }



  bool ValidateLogFile(
    const std::string& relativePath
    , fs::path& file
    , std::string& response
  )
  {
    std::string error;
    if (!ResolvePath(relativePath, file, error))
    {
      response += "error: ";
      response += error;
      return false;
    }

    std::vector<std::string> extensions = GetLogExtensions();
    std::unordered_set<std::string> extensionSet(extensions.begin(), extensions.end());
    if (!IsAllowedLogFile(file, extensionSet))
    {
      response += "error: file extension is not allowed";
      return false;
    }

    std::error_code ec;
    if (!fs::is_regular_file(file, ec))
    {
      response += "error: file is not found";
      return false;
    }

    return true;
  }

  bool ReadFileRange(
    const fs::path& file
    , uint64_t offset
    , uint64_t limit
    , std::string& response
  )
  {
    std::ifstream input(file, std::ios::binary);
    if (!input)
    {
      response += "error: unable to open file";
      return false;
    }

    input.seekg((std::streamoff)offset, std::ios::beg);

    std::vector<char> buffer((size_t)limit);
    input.read(buffer.data(), (std::streamsize)buffer.size());
    response.append(buffer.data(), (size_t)input.gcount());
    return true;
  }

  bool CommandRead(
    Logme::StringArray& arr
    , std::string& response
  )
  {
    if (arr.size() < 3)
    {
      response += "error: missing file path";
      return false;
    }

    size_t pathEnd = arr.size();
    uint64_t offset = 0;
    uint64_t limit = DEFAULT_READ_BYTES;

    if (arr.size() > 4 && IsUnsignedNumber(arr[arr.size() - 2]) && IsUnsignedNumber(arr[arr.size() - 1]))
    {
      offset = (uint64_t)std::strtoull(arr[arr.size() - 2].c_str(), nullptr, 10);
      limit = (uint64_t)std::strtoull(arr[arr.size() - 1].c_str(), nullptr, 10);
      pathEnd = arr.size() - 2;
    }

    if (limit == 0)
      limit = DEFAULT_READ_BYTES;

    if (limit > MAX_READ_BYTES)
      limit = MAX_READ_BYTES;

    fs::path file;
    std::string relativePath = JoinWords(arr, 2, pathEnd);
    if (!ValidateLogFile(relativePath, file, response))
      return false;

    uint64_t size = GetFileSize(file);
    if (offset > size)
      offset = size;

    std::ostringstream header;
    header << "LOGMEWEB-RANGE\t" << offset << "\t" << limit << "\t" << size << "\n";
    response += header.str();

    return ReadFileRange(file, offset, limit, response);
  }



  std::string EncodeBase64(const std::vector<char>& input)
  {
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    output.reserve(((input.size() + 2) / 3) * 4);

    for (size_t i = 0; i < input.size(); i += 3)
    {
      size_t remaining = input.size() - i;
      uint32_t octetA = (unsigned char)input[i];
      uint32_t octetB = remaining > 1 ? (unsigned char)input[i + 1] : 0;
      uint32_t octetC = remaining > 2 ? (unsigned char)input[i + 2] : 0;
      uint32_t triple = (octetA << 16) | (octetB << 8) | octetC;

      output.push_back(table[(triple >> 18) & 0x3F]);
      output.push_back(table[(triple >> 12) & 0x3F]);
      output.push_back(remaining > 1 ? table[(triple >> 6) & 0x3F] : '=');
      output.push_back(remaining > 2 ? table[triple & 0x3F] : '=');
    }

    return output;
  }

  bool ReadFileBytes(
    const fs::path& file
    , std::vector<char>& data
    , std::string& response
  )
  {
    std::ifstream input(file, std::ios::binary);
    if (!input)
    {
      response += "error: unable to open file";
      return false;
    }

    input.seekg(0, std::ios::end);
    std::streamoff size = input.tellg();
    if (size < 0)
    {
      response += "error: unable to get file size";
      return false;
    }

    input.seekg(0, std::ios::beg);
    data.resize((size_t)size);
    if (!data.empty())
      input.read(data.data(), (std::streamsize)data.size());

    return true;
  }

  bool CommandDownload(
    Logme::StringArray& arr
    , std::string& response
  )
  {
    if (arr.size() < 3)
    {
      response += "error: missing file path";
      return false;
    }

    fs::path file;
    std::string relativePath = JoinWords(arr, 2, arr.size());
    if (!ValidateLogFile(relativePath, file, response))
      return false;

    uint64_t size = GetFileSize(file);
    if (size > MAX_DOWNLOAD_BYTES)
    {
      response += "error: file is too large to download through control interface";
      return false;
    }

    std::vector<char> data;
    if (!ReadFileBytes(file, data, response))
      return false;

    std::ostringstream header;
    header << "LOGMEWEB-DOWNLOAD-B64\t" << data.size() << "\n";
    response += header.str();
    response += EncodeBase64(data);
    return true;
  }

  bool CommandTail(
    Logme::StringArray& arr
    , std::string& response
  )
  {
    if (arr.size() < 3)
    {
      response += "error: missing file path";
      return false;
    }

    size_t pathEnd = arr.size();
    uint64_t bytes = DEFAULT_TAIL_BYTES;

    if (arr.size() > 3 && IsUnsignedNumber(arr.back()))
    {
      bytes = (uint64_t)std::strtoull(arr.back().c_str(), nullptr, 10);
      pathEnd = arr.size() - 1;
    }

    if (bytes == 0)
      bytes = DEFAULT_TAIL_BYTES;

    if (bytes > MAX_READ_BYTES)
      bytes = MAX_READ_BYTES;

    fs::path file;
    std::string error;
    std::string relativePath = JoinWords(arr, 2, pathEnd);
    if (!ResolvePath(relativePath, file, error))
    {
      response += "error: ";
      response += error;
      return false;
    }

    std::vector<std::string> extensions = GetLogExtensions();
    std::unordered_set<std::string> extensionSet(extensions.begin(), extensions.end());
    if (!IsAllowedLogFile(file, extensionSet))
    {
      response += "error: file extension is not allowed";
      return false;
    }

    std::error_code ec;
    if (!fs::is_regular_file(file, ec))
    {
      response += "error: file is not found";
      return false;
    }

    uint64_t size = GetFileSize(file);
    uint64_t offset = size > bytes ? size - bytes : 0;

    std::ifstream input(file, std::ios::binary);
    if (!input)
    {
      response += "error: unable to open file";
      return false;
    }

    input.seekg((std::streamoff)offset, std::ios::beg);

    if (offset != 0)
    {
      std::string partial;
      std::getline(input, partial);
    }

    std::ostringstream out;
    out << input.rdbuf();
    response += out.str();

    return true;
  }
}

bool Logger::CommandLogs(Logme::StringArray& arr, std::string& response)
{
  if (arr.size() < 2 || arr[1] == "--info")
  {
    (void)CommandInfo(response);
    return true;
  }

  if (arr[1] == "--tree")
  {
    (void)CommandTree(arr, response);
    return true;
  }

  if (arr[1] == "--tail")
  {
    (void)CommandTail(arr, response);
    return true;
  }

  if (arr[1] == "--read")
  {
    (void)CommandRead(arr, response);
    return true;
  }

  if (arr[1] == "--download")
  {
    (void)CommandDownload(arr, response);
    return true;
  }

  response += "error: unsupported logs command";
  return true;
}
