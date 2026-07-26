#pragma once

namespace Logme
{
  enum class LogStatisticsSort
  {
    MESSAGE_BYTES,
    RECORDS
  };

  enum class LogFileStatisticsSort
  {
    WRITTEN_BYTES,
    WRITE_BATCHES,
    WRITE_ERRORS,
    DROPPED_BYTES
  };
}
