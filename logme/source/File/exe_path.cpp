#include <Logme/File/exe_path.h>

#include <errno.h>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#define _getcwd getcwd
#define MAX_PATH PATH_MAX
#endif

std::string Logme::AppendSlash(const std::string& folder)
{
  if (folder.empty())
    return folder;

  char ch = folder[folder.length() - 1];

#ifdef _WIN32  
  char slash = '\\';
#else
  char slash = '/';
#endif

  if (ch == slash)
    return folder;

  return folder + slash;
}

std::string Logme::GetExecutablePath()
{
  const size_t GROW = 512;
  std::vector<char> buffer(GROW);
  buffer[0] = '\0';

  while (true)
  {
    char* p = _getcwd(&buffer[0], (int)buffer.size());
    if (p)
      break;

    if (errno != ERANGE)
      break;

    buffer.resize(buffer.size() + GROW);
  }
  return AppendSlash(&buffer[0]);
}
