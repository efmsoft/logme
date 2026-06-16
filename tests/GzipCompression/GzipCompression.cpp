#include <Logme/File/CompressionManager.h>

#include <zlib.h>

#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

static void Check(
  bool condition
  , const std::string& message
)
{
  if (!condition)
  {
    throw std::runtime_error(message);
  }
}

static std::string ReadGzipFile(const std::filesystem::path& file)
{
  gzFile input = gzopen(file.string().c_str(), "rb");
  Check(input != nullptr, "failed to open gzip file");

  std::string result;
  char buffer[256];

  for (;;)
  {
    int read = gzread(input, buffer, sizeof(buffer));

    if (read < 0)
    {
      int error = 0;
      const char* text = gzerror(input, &error);
      std::string message = "failed to read gzip file";

      if (text)
      {
        message += ": ";
        message += text;
      }

      gzclose(input);
      throw std::runtime_error(message);
    }

    if (read == 0)
      break;

    result.append(buffer, static_cast<std::size_t>(read));
  }

  int rc = gzclose(input);
  Check(rc == Z_OK, "failed to close gzip file");

  return result;
}

static std::filesystem::path MakeTestDirectory()
{
  auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  auto dir = std::filesystem::temp_directory_path()
    / ("logme-gzip-compression-test-" + std::to_string(now));

  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
  std::filesystem::create_directories(dir, ec);
  Check(!ec, "failed to create test directory");

  return dir;
}

static void WriteTextFile(
  const std::filesystem::path& file
  , const std::string& text
)
{
  std::ofstream output(file, std::ios::binary);
  Check(output.good(), "failed to open source file");

  output.write(text.data(), static_cast<std::streamsize>(text.size()));
  Check(output.good(), "failed to write source file");
}

static void TestGzipCompression()
{
  auto dir = MakeTestDirectory();
  auto source = dir / "completed.log";
  auto compressed = dir / "completed.log.gz";

  const std::string text =
    "line 1\n"
    "line 2\n"
    "line 3\n";

  WriteTextFile(source, text);

  Logme::CompressionManagerFactory factory(
    [](const std::string& file)
    {
      (void)file;
      return false;
    }
  );

  auto registration = factory.RegisterUser();
  Check(registration != nullptr, "failed to register compression user");
  Check(registration->IsActive(), "compression registration is inactive");

  registration->Submit(source.string());
  registration.reset();
  factory.SetStopping();

  Check(!std::filesystem::exists(source), "source file was not removed");
  Check(std::filesystem::exists(compressed), "compressed file was not created");
  Check(ReadGzipFile(compressed) == text, "compressed file content mismatch");

  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
}

int main()
{
  try
  {
    TestGzipCompression();
    return 0;
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return 1;
  }
}
