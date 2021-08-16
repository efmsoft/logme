#include <Logme/Convert.h>

#include <stdlib.h>
#include <vector>

using namespace Logme;

static std::string FromWideString(const wchar_t* src)
{
  size_t n;
#ifdef _WIN32
  auto e = wcstombs_s(&n, nullptr, 0, src, 0) + 1;
  if (e)
    n = -1;
#else
  n = wcstombs(nullptr, src, 0) + 1;
#endif
  if (n == -1)
    return "";

  std::vector<char> buffer(n);
#ifdef _WIN32
  e = wcstombs_s(&n, &buffer[0], n, src, n);
  if (e)
    n = -1;
#else
  n = wcstombs(&buffer[0], src, n);
#endif
  if (n == -1)
    return "";

  return &buffer[0];
}

static std::wstring FromString(const char* src)
{
  size_t n;
#ifdef _WIN32
  auto e = mbstowcs_s(&n, nullptr, 0, src, 0) + 1;
  if (e)
    n = -1;
#else
  n = mbstowcs(nullptr, src, 0) + 1;
#endif

  if (n == -1)
    return L"";

  std::vector<wchar_t> buffer(n);
#ifdef _WIN32
  e = mbstowcs_s(&n, &buffer[0], n, src, n);
  if (e)
    n = -1;
#else
  n = mbstowcs(&buffer[0], src, n);
#endif

  if (n == -1)
    return L"";

  return &buffer[0];
}

std::string Logme::ToStdString(const std::string& src)
{
  return src;  
}

std::string Logme::ToStdString(const char* src)
{
  return src;  
}

std::string Logme::ToStdString(const std::wstring& src)
{
  return FromWideString(src.c_str());
}

std::string Logme::ToStdString(const wchar_t* src)
{
  return FromWideString(src);
}

std::wstring Logme::ToStdWString(const std::string& src)
{
  return FromString(src.c_str());  
}

std::wstring Logme::ToStdWString(const char* src)
{
  return FromString(src); 
}

std::wstring Logme::ToStdWString(const std::wstring& src)
{
  return src;  
}

std::wstring Logme::ToStdWString(const wchar_t* src)
{
  return src;  
}
