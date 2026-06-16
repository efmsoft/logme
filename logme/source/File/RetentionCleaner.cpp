#include <algorithm>
#include <chrono>
#include <filesystem>
#include <regex>
#include <string>
#include <vector>

#include <Logme/File/RetentionCleaner.h>
#include <Logme/Logme.h>

namespace fs = std::filesystem;

namespace
{
  struct FileInfo
  {
    fs::path Path;
    std::string PathName;
    std::filesystem::file_time_type LastWrite;
    std::uint64_t Size = 0;
    bool Deleted = false;
  };

  bool IsSameFileName(
    const std::string& left
    , const std::string& right
  )
  {
    return left == right;
  }

  std::vector<FileInfo> FindMatchingFiles(
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
      const std::string pathName = path.string();
      if (!std::regex_match(pathName, pattern))
        continue;

      auto lastWrite = entry.last_write_time(ec);
      if (ec)
      {
        ec.clear();
        continue;
      }

      std::uint64_t size = static_cast<std::uint64_t>(entry.file_size(ec));
      if (ec)
      {
        ec.clear();
        size = 0;
      }

      FileInfo info;
      info.Path = path;
      info.PathName = pathName;
      info.LastWrite = lastWrite;
      info.Size = size;

      matchedFiles.push_back(info);
    }

    return matchedFiles;
  }

  void SortNewestFirst(std::vector<FileInfo>& files)
  {
    std::sort(
      files.begin()
      , files.end()
      , [](const FileInfo& a, const FileInfo& b)
      {
        if (a.LastWrite != b.LastWrite)
          return a.LastWrite > b.LastWrite;

        return a.PathName < b.PathName;
      }
    );
  }

  void SortOldestFirst(std::vector<FileInfo*>& files)
  {
    std::sort(
      files.begin()
      , files.end()
      , [](const FileInfo* a, const FileInfo* b)
      {
        if (a->LastWrite != b->LastWrite)
          return a->LastWrite < b->LastWrite;

        return a->PathName < b->PathName;
      }
    );
  }

  bool DeleteFile(
    FileInfo& file
    , const std::string& keep
  )
  {
    if (file.Deleted)
      return true;

    if (IsSameFileName(file.PathName, keep))
      return false;

    std::error_code ec;
    fs::remove(file.Path, ec);

    if (ec)
    {
      LogmeE(CHINT, "failed to delete: %s", file.PathName.c_str());
      return false;
    }

    file.Deleted = true;
    return true;
  }

  std::size_t CountExisting(const std::vector<FileInfo>& files)
  {
    std::size_t count = 0;

    for (const auto& file : files)
    {
      if (!file.Deleted)
        ++count;
    }

    return count;
  }

  std::uint64_t CalculateTotalSize(const std::vector<FileInfo>& files)
  {
    std::uint64_t total = 0;

    for (const auto& file : files)
    {
      if (!file.Deleted)
        total += file.Size;
    }

    return total;
  }

  std::vector<FileInfo*> GetOldestExistingFiles(std::vector<FileInfo>& files)
  {
    std::vector<FileInfo*> result;

    for (auto& file : files)
    {
      if (!file.Deleted)
        result.push_back(&file);
    }

    SortOldestFirst(result);
    return result;
  }

  void ApplyMaxFiles(
    std::vector<FileInfo>& files
    , const std::string& keep
    , std::size_t limit
  )
  {
    if (limit == 0)
      return;

    std::size_t remaining = CountExisting(files);
    if (remaining <= limit)
      return;

    auto oldestFiles = GetOldestExistingFiles(files);
    for (FileInfo* file : oldestFiles)
    {
      if (remaining <= limit)
        break;

      if (!DeleteFile(*file, keep))
        continue;

      --remaining;
    }
  }

  void ApplyMaxAge(
    std::vector<FileInfo>& files
    , const std::string& keep
    , std::uint64_t maxAgeMs
  )
  {
    if (maxAgeMs == 0)
      return;

    auto maxAge = std::chrono::milliseconds(maxAgeMs);
    auto now = std::filesystem::file_time_type::clock::now();

    for (auto& file : files)
    {
      if (file.Deleted)
        continue;

      if (IsSameFileName(file.PathName, keep))
        continue;

      if (now - file.LastWrite <= maxAge)
        continue;

      DeleteFile(file, keep);
    }
  }

  void ApplyMaxTotalSize(
    std::vector<FileInfo>& files
    , const std::string& keep
    , std::uint64_t limit
  )
  {
    if (limit == 0)
      return;

    std::uint64_t total = CalculateTotalSize(files);
    if (total <= limit)
      return;

    auto oldestFiles = GetOldestExistingFiles(files);
    for (FileInfo* file : oldestFiles)
    {
      if (total <= limit)
        break;

      const std::uint64_t size = file->Size;
      if (!DeleteFile(*file, keep))
        continue;

      if (size > total)
        total = 0;
      else
        total -= size;
    }
  }
}

void Logme::CleanFiles(
  const std::regex& pattern
  , const std::string& keep
  , const RetentionOptions& options
)
{
  if (keep.empty() || options.IsEmpty())
    return;

  fs::path file(keep);
  std::vector<FileInfo> matchedFiles = FindMatchingFiles(
    pattern
    , file.parent_path()
  );

  if (matchedFiles.empty())
    return;

  SortNewestFirst(matchedFiles);

  ApplyMaxFiles(matchedFiles, keep, options.MaxFiles);
  ApplyMaxAge(matchedFiles, keep, options.MaxAgeMs);
  ApplyMaxTotalSize(matchedFiles, keep, options.MaxTotalSize);
}

