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

  for (const auto& entry : fs::directory_iterator(dir))
  {
    if (!entry.is_regular_file())
      continue;

    const auto& path = entry.path();
    if (std::regex_match(path.string(), pattern))
    { 
      FileInfo info{ path, entry.last_write_time() };
      matchedFiles.push_back(info);
    }
  }

  return matchedFiles;
}

static void SortFilesByTime(std::vector<FileInfo>& files)
{
  std::sort(
    files.begin()
    , files.end()
    , [](const FileInfo& a, const FileInfo& b) {
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
  if (files.size() <= limit)
    return;

  size_t toDelete = files.size() - limit;
  for (size_t i = 0; i < toDelete; ++i)
  {
    std::string path = files[i].Path.string();

    if (path == keep)
      continue;

    std::error_code ec;
    fs::remove(files[i].Path, ec);
 
    if (ec)
    {
      LogmeE(CHINT, "Failed to delete: %s", path.c_str());
    }
  }
}

void Logme::CleanFiles(
  const std::regex& pattern
  , const std::string& keep
  , size_t limit
)
{
  fs::path file(keep);
  std::vector<FileInfo> matchedFiles = FindMatchingFiles(pattern, file.parent_path());

  SortFilesByTime(matchedFiles);
  DeleteExcessFiles(matchedFiles, keep, limit);
}