#include <Logme/Backend/FileBackend.h>
#include <Logme/Channel.h>
#include <Logme/Context.h>
#include <Logme/File/exe_path.h>
#include <Logme/Logger.h>
#include <Logme/Template.h>
#include <Logme/Types.h>

#include <cassert>
#include <stdint.h>
#include <vector>

#ifndef _WIN32
#include <sys/file.h>
#include <unistd.h>

#define _lseek lseek
#define _read read
#define _write write
#define sprintf_s sprintf
#else
#include <io.h>
#define ftruncate _chsize
#endif

using namespace Logme;

FileBackend::FileBackend(ChannelPtr owner)
  : Backend(owner)
  , Append(true)
  , MaxSize(MAX_SIZE_DEFAULT)
{
}

FileBackend::~FileBackend()
{
  CloseLog();
}

void FileBackend::SetMaxSize(size_t size)
{
  assert(size > 1024);
  MaxSize = size;
}

void FileBackend::SetAppend(bool append)
{
  Append = append;
}

bool FileBackend::CreateLog(const char* name)
{
  CloseLog();
  Name.clear();

  if (name)
    Name = name;

  return Open(Append);
}

void FileBackend::CloseLog()
{
  Close();
}

void FileBackend::WriteLine(Context& context, const char* line)
{
  if (File == -1)
    return;

  Truncate();

  int nc;
  const char* buffer = context.Apply(Owner->GetFlags(), line, nc);

  Write(buffer, nc);
}

void FileBackend::Display(Context& context, const char* line)
{
  WriteLine(context, line);
}

void FileBackend::Truncate()
{
  if (File == -1)
    return;

  auto rc = _lseek(File, 0, SEEK_END);
  if (rc == -1)
  {
    assert(!"FileBackend::Truncate(): lseek failed");
    return;
  }

  if (rc < MaxSize)
    return;

  uint32_t readPos = uint32_t(MaxSize / 2);
  uint32_t readSize = rc - readPos;

  std::vector<char> data(readSize);
  rc = _lseek(File, readPos, SEEK_SET);
  if (rc == -1)
  {
    assert(!"FileBackend::Truncate(): lseek failed");
    return;
  }

  if (_read(File, &data[0], readSize) == -1)
  {
    assert(!"FileBackend::Truncate(): read failed");
    return;
  }

  const char* p = &data[0];
  while (*p && *p != '\n')
    p++;

  if (*p++)
  {
    uint32_t n = uint32_t(p - &data[0]);

    rc = _lseek(File, 0, SEEK_SET);
    if (rc == -1)
      assert(!"FileBackend::Truncate(): lseek failed");

    char buffer[128];
    sprintf_s(buffer, "--- dropped %i characters\n", readPos + n);
    if (_write(File, buffer, (int)strlen(buffer)) == -1)
      assert(!"FileBackend::Truncate(): write failed");

    if (_write(File, p, readSize - n) == -1)
      assert(!"FileBackend::Truncate(): write failed");

    if (ftruncate(File, _lseek(File, 0, SEEK_CUR)) == -1)
      assert(!"FileBackend::Truncate(): ftruncate failed");
  }

  rc = _lseek(File, 0, SEEK_END);
  if (rc == -1)
    assert(!"FileBackend::Truncate(): SetFilePointer failed");
}

std::string FileBackend::GetPathName(int index)
{
  if (index < 0 || index > 1)
    return "";

  ProcessTemplateParam param;
  std::string name = ProcessTemplate(Name.c_str(), param);
  std::string pathName = name;

  if (!IsAbsolutePath(pathName))
    pathName = Owner->GetOwner()->GetHomeDirectory() + name;

  return pathName;   
}
