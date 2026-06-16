#include <algorithm>
#include <filesystem>
#include <regex>
#include <vector>

#include <Logme/Logme.h>

#include "CleanOldest.h"

namespace fs = std::filesystem;

struct FileInfo
{
  fs::path Path;
  std::filesystem::file_time_type LastWrite;
};

static std::vector<FileInfo> FindMatchingFiles(
  const std::regex& pattern
  , const fs::path& dir
)
{
  std::vector<FileInfo> matchedFiles;

  if (dir.empty())
    return matchedFiles;

  std::error_code ec;
  if (!fs::exists(dir, ec) || ec)
    return matchedFiles;

  fs::directory_iterator it(dir, ec);
  if (ec)
    return matchedFiles;

  for (const auto& entry : it)
  {
    if (!entry.is_regular_file(ec) || ec)
    {
      ec.clear();
      continue;
    }

    const auto& path = entry.path();
    if (!std::regex_match(path.string(), pattern))
      continue;

    auto lastWrite = entry.last_write_time(ec);
    if (ec)
    {
      ec.clear();
      continue;
    }

    FileInfo info{ path, lastWrite };
    matchedFiles.push_back(info);
  }

  return matchedFiles;
}

static void SortFilesByTime(std::vector<FileInfo>& files)
{
  std::sort(
    files.begin()
    , files.end()
    , [](const FileInfo& a, const FileInfo& b)
    {
      return a.LastWrite < b.LastWrite;
    }
  );
}

static void DeleteExcessFiles(
  std::vector<FileInfo>& files
  , const std::string& keep
  , size_t limit
)
{
  if (limit == 0 || files.size() <= limit)
    return;

  size_t remaining = files.size();

  for (const auto& file : files)
  {
    if (remaining <= limit)
      break;

    std::string path = file.Path.string();
    if (path == keep)
      continue;

    std::error_code ec;
    fs::remove(file.Path, ec);

    if (ec)
    {
      LogmeE(CHINT, "failed to delete: %s", path.c_str());
      continue;
    }

    --remaining;
  }
}

void Logme::CleanFiles(
  const std::regex& pattern
  , const std::string& keep
  , size_t limit
)
{
  if (limit == 0 || keep.empty())
    return;

  fs::path file(keep);
  std::vector<FileInfo> matchedFiles = FindMatchingFiles(
    pattern
    , file.parent_path()
  );

  SortFilesByTime(matchedFiles);
  DeleteExcessFiles(matchedFiles, keep, limit);
}
