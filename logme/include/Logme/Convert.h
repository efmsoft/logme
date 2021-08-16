#pragma once

#include <string>

namespace Logme
{
  std::string ToStdString(const std::string& src);
  std::string ToStdString(const char* src);
  std::string ToStdString(const std::wstring& src);
  std::string ToStdString(const wchar_t* src);

  std::wstring ToStdWString(const std::string& src);
  std::wstring ToStdWString(const char* src);
  std::wstring ToStdWString(const std::wstring& src);
  std::wstring ToStdWString(const wchar_t* src);
}
