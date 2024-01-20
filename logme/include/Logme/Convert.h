#pragma once

#include <string>

#include <Logme/Types.h>

namespace Logme
{
  LOGMELNK std::string ToStdString(const std::string& src);
  LOGMELNK std::string ToStdString(const char* src);
  LOGMELNK std::string ToStdString(const std::wstring& src);
  LOGMELNK std::string ToStdString(const wchar_t* src);

  LOGMELNK std::wstring ToStdWString(const std::string& src);
  LOGMELNK std::wstring ToStdWString(const char* src);
  LOGMELNK std::wstring ToStdWString(const std::wstring& src);
  LOGMELNK std::wstring ToStdWString(const wchar_t* src);
}
