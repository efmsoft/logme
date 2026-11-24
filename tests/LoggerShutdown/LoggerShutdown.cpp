#include <gtest/gtest.h>

#include <Logme/Backend/FileBackend.h>
#include <Logme/Logme.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

namespace
{
  const int LineCount = 20000;

  std::filesystem::path LogPath()
  {
    // A fixed name: on Windows the death test child runs the test again
    // from the start, so both processes have to arrive at the same file.
    return std::filesystem::temp_directory_path() / "logme-logger-shutdown.log";
  }

  int CountLines(const std::filesystem::path& path, const std::string& text)
  {
    std::ifstream input(path);
    int count = 0;

    for (std::string line; std::getline(input, line);)
    {
      if (line.find(text) != std::string::npos)
        count++;
    }

    return count;
  }

  // Runs in the death test child: logs, calls Shutdown() and leaves at once,
  // without static destructors, the way a service process can be terminated
  // right after it reports SERVICE_STOPPED.
  [[noreturn]] void LogShutdownAndExit(const std::filesystem::path& path)
  {
    {
      Logme::ID id{"shutdown"};
      auto ch = Logme::Instance->CreateChannel(id, Logme::OutputFlags(), Logme::LEVEL_DEBUG);

      auto file = std::make_shared<Logme::FileBackend>(ch);
      file->SetAppend(false);
      file->SetMaxSize(64 * 1024 * 1024);
      if (!file->CreateLog(path.string().c_str()))
        std::_Exit(2);

      ch->AddBackend(file);

      // The file backend is asynchronous: the records stay queued until the
      // file manager thread writes them, which is not due yet (FlushAfter).
      for (int i = 0; i < LineCount; ++i)
        LogmeI(ch, "shutdown test line %d", i);
    }

    Logme::Logger::Shutdown();
    std::_Exit(0);
  }
}

// Logger::Shutdown() must not return before the queued file output is written.
TEST(LoggerShutdown, WritesQueuedFileOutputBeforeReturning)
{
  const auto path = LogPath();
  std::error_code ec;
  std::filesystem::remove(path, ec);

  EXPECT_EXIT(LogShutdownAndExit(path), ::testing::ExitedWithCode(0), "");
  EXPECT_EQ(CountLines(path, "shutdown test line "), LineCount);

  std::filesystem::remove(path, ec);
}
