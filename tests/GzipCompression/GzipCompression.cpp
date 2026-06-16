#include <Logme/File/CompressionManager.h>

#include <zlib.h>

#include <assert.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

static std::string ReadGzipFile(const std::filesystem::path& file)
{
  gzFile input = gzopen(file.string().c_str(), "rb");
  assert(input != nullptr);

  std::string result;
  char buffer[256];

  for (;;)
  {
    int read = gzread(input, buffer, sizeof(buffer));

    if (read < 0)
    {
      int error = 0;
      gzerror(input, &error);
      (void)error;
      assert(false);
    }

    if (read == 0)
      break;

    result.append(buffer, static_cast<std::size_t>(read));
  }

  int rc = gzclose(input);
  assert(rc == Z_OK);

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
  assert(!ec);

  return dir;
}

static void WriteTextFile(
  const std::filesystem::path& file
  , const std::string& text
)
{
  std::ofstream output(file, std::ios::binary);
  assert(output.good());

  output.write(text.data(), static_cast<std::streamsize>(text.size()));
  assert(output.good());
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
  assert(registration && registration->IsActive());

  registration->Submit(source.string());
  registration.reset();
  factory.SetStopping();

  assert(!std::filesystem::exists(source));
  assert(std::filesystem::exists(compressed));
  assert(ReadGzipFile(compressed) == text);

  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
}

int main()
{
  TestGzipCompression();
  return 0;
}
