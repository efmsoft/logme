#pragma once

#include <ctime>
#include <regex>
#include <stdint.h>
#include <string>

namespace Logme
{
  class FileArchivePolicy
  {
  public:
    FileArchivePolicy();

    void Configure(
      const std::string& archiveTemplate
      , const std::string& homeDirectory
      , bool gzipCompression
    );

    bool IsEnabled() const;
    const std::string& GetTemplate() const;

    void SetTime(std::time_t archiveTime);
    std::time_t GetTime() const;

    std::string BuildName(uint64_t index) const;
    std::string BuildName(
      uint64_t index
      , std::time_t archiveTime
    ) const;
    std::string BuildCleanPattern() const;
    std::string TakeName(std::time_t archiveTime);

  private:
    std::regex BuildIndexPattern(std::time_t archiveTime) const;
    bool NameExists(const std::string& archive) const;
    uint64_t FindLastIndex(std::time_t archiveTime) const;
    void RecoverIndex();

    std::string ArchiveTemplate;
    std::string HomeDirectory;
    bool GzipCompression;
    uint64_t ArchiveIndex;
    std::time_t ArchiveTime;
  };
}
