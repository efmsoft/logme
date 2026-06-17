#include "FileArchivePolicy.h"

#include <charconv>
#include <filesystem>
#include <system_error>

#include <Logme/File/exe_path.h>
#include <Logme/Template.h>
#include <Logme/Types.h>
#include <Logme/Utils.h>

namespace
{
  bool TryParseUInt64(
    const std::string& text
    , uint64_t& value
  )
  {
    if (text.empty())
    {
      return false;
    }

    uint64_t result = 0;
    const char* first = text.data();
    const char* last = text.data() + text.size();

    auto parseResult = std::from_chars(first, last, result, 10);
    if (parseResult.ec != std::errc())
    {
      return false;
    }

    if (parseResult.ptr != last)
    {
      return false;
    }

    value = result;
    return true;
  }

  std::string FormatArchiveTime(
    std::time_t value
    , const char* format
  )
  {
    struct tm tmValue {};
#ifdef _WIN32
    localtime_s(&tmValue, &value);
    struct tm* localTime = &tmValue;
#else
    struct tm* localTime = localtime_r(&value, &tmValue);
#endif

    if (!localTime)
    {
      return std::string();
    }

    char buffer[32];
    strftime(buffer, sizeof(buffer), format, localTime);
    return buffer;
  }

  std::string ApplyArchiveTime(
    const std::string& text
    , std::time_t archiveTime
  )
  {
    std::string result = Logme::ReplaceAll(
      text
      , Logme::TDATETIME
      , FormatArchiveTime(archiveTime, "%Y-%m-%d-%H-%M-%S")
    );

    return Logme::ReplaceAll(
      result
      , Logme::TDATE
      , FormatArchiveTime(archiveTime, "%Y-%m-%d")
    );
  }
}

namespace Logme
{
  FileArchivePolicy::FileArchivePolicy()
    : GzipCompression(false)
    , ArchiveIndex(0)
    , ArchiveTime(0)
  {
  }

  void FileArchivePolicy::Configure(
    const std::string& archiveTemplate
    , const std::string& homeDirectory
    , bool gzipCompression
  )
  {
    ArchiveTemplate = archiveTemplate;
    HomeDirectory = homeDirectory;
    GzipCompression = gzipCompression;
    ArchiveIndex = 0;
    ArchiveTime = 0;
  }

  bool FileArchivePolicy::IsEnabled() const
  {
    return !ArchiveTemplate.empty();
  }

  const std::string& FileArchivePolicy::GetTemplate() const
  {
    return ArchiveTemplate;
  }

  void FileArchivePolicy::SetTime(std::time_t archiveTime)
  {
    if (archiveTime == 0)
    {
      archiveTime = time(nullptr);
    }

    if (ArchiveTemplate.empty())
    {
      ArchiveTime = archiveTime;
      ArchiveIndex = 0;
      return;
    }

    std::string oldProbe;
    if (ArchiveTime != 0)
    {
      oldProbe = BuildName(0, ArchiveTime);
    }

    ArchiveTime = archiveTime;

    std::string newProbe = BuildName(0, ArchiveTime);
    if (oldProbe != newProbe)
    {
      RecoverIndex();
    }
  }

  std::time_t FileArchivePolicy::GetTime() const
  {
    return ArchiveTime;
  }

  std::string FileArchivePolicy::BuildName(uint64_t index) const
  {
    return BuildName(index, ArchiveTime);
  }

  std::string FileArchivePolicy::BuildName(
    uint64_t index
    , std::time_t archiveTime
  ) const
  {
    ProcessTemplateParam param(
      static_cast<uint32_t>(TEMPLATE_ALL)
      & ~static_cast<uint32_t>(TEMPLATE_DATE_AND_TIME)
    );

    std::string archive = ProcessTemplate(ArchiveTemplate.c_str(), param);
    archive = ApplyArchiveTime(archive, archiveTime);
    archive = ReplaceAll(archive, "{index}", std::to_string(index));

    if (!IsAbsolutePath(archive))
    {
      archive = HomeDirectory + archive;
    }

    return archive;
  }

  std::string FileArchivePolicy::BuildCleanPattern() const
  {
    ProcessTemplateParam param(
      static_cast<uint32_t>(TEMPLATE_ALL)
      & ~static_cast<uint32_t>(TEMPLATE_DATE_AND_TIME)
    );

    std::string re = ProcessTemplate(ArchiveTemplate.c_str(), param);
    re = ReplaceAll(re, "{index}", TDATETIME);

    if (!IsAbsolutePath(re))
    {
      re = HomeDirectory + re;
    }

    return ReplaceDatetimePlaceholders(re, ".+");
  }

  std::string FileArchivePolicy::TakeName(std::time_t archiveTime)
  {
    SetTime(archiveTime);

    for (uint64_t i = ArchiveIndex + 1;; ++i)
    {
      std::string archive = BuildName(i, ArchiveTime);
      if (!NameExists(archive))
      {
        ArchiveIndex = i;
        return archive;
      }
    }
  }

  std::regex FileArchivePolicy::BuildIndexPattern(std::time_t archiveTime) const
  {
    const std::string indexMarker = "\x01INDEX\x02";

    ProcessTemplateParam param(
      static_cast<uint32_t>(TEMPLATE_ALL)
      & ~static_cast<uint32_t>(TEMPLATE_DATE_AND_TIME)
    );

    std::string re = ProcessTemplate(ArchiveTemplate.c_str(), param);
    re = ApplyArchiveTime(re, archiveTime);
    re = ReplaceAll(re, "{index}", indexMarker);

    if (!IsAbsolutePath(re))
    {
      re = HomeDirectory + re;
    }

    re = ReplaceDatetimePlaceholders(re, ".+");
    re = ReplaceAll(re, indexMarker, "([0-9]+)");
    re += "(\\.gz)?";

    return std::regex(re);
  }

  bool FileArchivePolicy::NameExists(const std::string& archive) const
  {
    std::error_code ec;
    if (std::filesystem::exists(archive, ec))
    {
      return true;
    }

    if (ec)
    {
      return true;
    }

    ec.clear();
    if (std::filesystem::exists(archive + ".gz", ec))
    {
      return true;
    }

    return static_cast<bool>(ec);
  }

  uint64_t FileArchivePolicy::FindLastIndex(std::time_t archiveTime) const
  {
    if (ArchiveTemplate.empty())
    {
      return 0;
    }

    std::filesystem::path archiveSample(BuildName(0, archiveTime));
    std::filesystem::path dir = archiveSample.parent_path();
    if (dir.empty())
    {
      return 0;
    }

    std::error_code ec;
    if (!std::filesystem::exists(dir, ec) || ec)
    {
      return 0;
    }

    std::filesystem::directory_iterator it(dir, ec);
    if (ec)
    {
      return 0;
    }

    std::regex pattern = BuildIndexPattern(archiveTime);
    uint64_t maxIndex = 0;

    for (const auto& entry : it)
    {
      if (!entry.is_regular_file(ec) || ec)
      {
        ec.clear();
        continue;
      }

      std::smatch match;
      std::string pathName = entry.path().string();
      if (!std::regex_match(pathName, match, pattern) || match.size() < 2)
      {
        continue;
      }

      uint64_t index = 0;
      if (!TryParseUInt64(match[1].str(), index))
      {
        continue;
      }

      if (index > maxIndex)
      {
        maxIndex = index;
      }
    }

    return maxIndex;
  }

  void FileArchivePolicy::RecoverIndex()
  {
    ArchiveIndex = FindLastIndex(ArchiveTime);
  }
}
