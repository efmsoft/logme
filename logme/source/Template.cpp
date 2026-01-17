#include <ctime>
#include <regex>
#include <string.h>
#include <time.h>

#include <Logme/File/exe_path.h>
#include <Logme/Logme.h>
#include <Logme/Template.h>
#include <Logme/Utils.h>

#if defined(__GNUC__) && !defined(__DJGPP__)
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#define PATH_SEPARATOR '/'
#elif defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#define PATH_SEPARATOR '\\'
 #include <windows.h>
#else
#define PATH_SEPARATOR '/'
#include <time.h>
 #endif

#ifdef __APPLE__
#include <libproc.h>
#endif

#if defined(__APPLE__)
#define MAX_PATH PROC_PIDPATHINFO_MAXSIZE
#elif defined(__sun__)
#include <unistd.h>
#define MAX_PATH FILENAME_MAX
#elif defined(__linux__)
#include <linux/limits.h>
#define MAX_PATH PATH_MAX
#endif
 
using namespace Logme;

const char* Logme::TPID = "{pid}";
const char* Logme::TPNAME ="{pname}";
const char* Logme::TDATE = "{date}";
const char* Logme::TDATETIME = "{datetime}";
const char* Logme::TTARGET = "{target}";
const char* Logme::TEXEPATH = "{exepath}";

#if defined(__linux__) || defined(__APPLE__) || defined(__sun__)
static int GetModuleFileNameA(int, char* buff, size_t maxLen)
{
#if defined(__linux__)
  int count = readlink("/proc/self/exe", buff, maxLen - 1);
  if (count == -1)
    count = 0;
#elif defined(__sun__)
  char exeLink[MAX_PATH + 1];
  snprintf(exeLink, MAX_PATH + 1, "/proc/%d/path/a.out", getpid());
  int count = readlink(exeLink, buff, maxLen - 1);
  if (count == -1)
    count = 0;
#else
  int count = proc_pidpath(getpid(), buff, maxLen - 1);
  if (count <= 0)
    count = 0;
#endif

  buff[count] = 0;
  return count;
}
#endif

static std::string GetExeDirectory()
{
  char buff[MAX_PATH + 1];
  GetModuleFileNameA(0, buff, MAX_PATH);
  buff[MAX_PATH] = '\0';

  char* e = strrchr(buff, PATH_SEPARATOR);
  if (e)
    *e = '\0';

  return buff;
}

static std::string GetExeName()
{
  char buff[MAX_PATH + 1];
  GetModuleFileNameA(0, buff, MAX_PATH);
  buff[MAX_PATH] = '\0';

  char* e = strrchr(buff, PATH_SEPARATOR);
  return e ? e + 1 : buff;
} 

std::string Logme::ReplaceDatetimePlaceholders(
  const std::string& where
  , const std::string& on
)
{
  static const std::regex special_chars(R"([.^$|()\\[\]*+?])");
  std::string re(std::regex_replace(where, special_chars, R"(\$&)"));
  
  re = ReplaceAll(re, TDATE, ".+");
  re = ReplaceAll(re, TDATETIME, ".+");

  return ReplaceAll(re, ".+.+", ".+");
}

std::string Logme::ProcessTemplate(
  const char* p
  , const ProcessTemplateParam& param
  , uint32_t* notProcessed
)
{
  static const size_t TPID_L = strlen(TPID);
  static const size_t TPNAME_L = strlen(TPNAME);
  static const size_t TDATE_L = strlen(TDATE);
  static const size_t TDATETIME_L = strlen(TDATETIME);
  static const size_t TTARGET_L = strlen(TTARGET);
  static const size_t TEXEPATH_L = strlen(TEXEPATH);

  bool ftemplate = false;
  const char* tstr = p;

  if (notProcessed)
    *notProcessed = 0;

  std::string name;
  while (p && *p)
  {
    if (*p == '{')
    {
      const char* e = strchr(p, '}');
      if (p[1] == '%' && e)                 // If reference on a env variable
      {
        p += 2;                             // {%

        std::string var(p, e - p);
        if (var.find('%') != std::string::npos)
        {
          name += "{%";
          name += var;
          name += "}";
        }
        else
        {
          uint32_t m = 0;
          std::string v = EnvGetVar(var.c_str());

          // The variable value string can also contain 
          // substitution fields
          v = ProcessTemplate(v.c_str(), param, &m);

          ftemplate = true;
          name += v;
        }
          
        p = e + 1;                          // next char after }
        continue;
      }
      else if (strncmp(p, TPID, TPID_L) == 0)
      {
        if (param.Flags & TEMPLATE_PID)
        {
          p += TPID_L;
          name += dword2str(GetCurrentProcessId());
          continue;
        }

        if (notProcessed)
          *notProcessed |= TEMPLATE_PID;

        ftemplate = true;
      }
      else if (strncmp(p, TPNAME, TPNAME_L) == 0)
      {
        if (param.Flags & TEMPLATE_PNAME)
        {
          p += TPNAME_L;

          std::string exe = GetExeName();
          name += exe.substr(0, exe.find_last_of('.'));
          continue;
        }

        if (notProcessed)
          *notProcessed |= TEMPLATE_PNAME;
      
        ftemplate = true;
      }
      else if (strncmp(p, TDATETIME, TDATETIME_L) == 0)
      {
        if (param.Flags & TEMPLATE_PDATETIME)
        {
          p += TDATETIME_L;

          const time_t now = time(0);
          struct tm tmstg {};

#ifdef _WIN32
          struct tm* t = &tmstg;
          localtime_s(t, &now);
#else
          struct tm * t = localtime_r(&now, &tmstg);
#endif
          char buffer[32];
          strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H-%M-%S", t);

          name += buffer;
          continue;
        }

        if (notProcessed)
          *notProcessed |= TEMPLATE_PDATETIME;

        ftemplate = true;
      }
      else if (strncmp(p, TDATE, TDATE_L) == 0)
      {
        if (param.Flags & TEMPLATE_PDATE)
        {
          p += TDATE_L;

          const time_t now = time(0);
          struct tm tmstg {};

#ifdef _WIN32
          struct tm* t = &tmstg;
          localtime_s(t, &now);
#else
          struct tm* t = localtime_r(&now, &tmstg);
#endif
          char buffer[32];
          strftime(buffer, sizeof(buffer), "%Y-%m-%d", t);

          name += buffer;
          continue;
        }

        if (notProcessed)
          *notProcessed |= TEMPLATE_PDATE;
      }
      else if (strncmp(p, TTARGET, TTARGET_L) == 0)
      {
        if (param.Flags & TEMPLATE_TARGET)
        {
          p += TTARGET_L;

          if (!param.TargetChannel)
            name += "0";
          else
            name += param.TargetChannel;

          continue;
        }

        if (notProcessed)
          *notProcessed |= TEMPLATE_TARGET;

        ftemplate = true;
      }
      else if (strncmp(p, TEXEPATH, TEXEPATH_L) == 0)
      {
        if (param.Flags & TEMPLATE_EXEPATH)
        {
          p += TEXEPATH_L;

          name += GetExecutablePath();
          continue;
        }

        if (notProcessed)
          *notProcessed |= TEMPLATE_EXEPATH;

        ftemplate = true;
      }
    }

    name += *p++;
  }

  (void)ftemplate;
  LogmeI_If(ftemplate, CHINT, "ProcessTemplate: %s -> %s", tstr, name.c_str());

  return name;
}
 