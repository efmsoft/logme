#pragma once

#include <string>

namespace Logme
{
  template<typename T1> 
  std::string ArgumentList(
    const char* name1, const T1& t1
  )
  {
    std::string FormatValue(const T1&);
    return std::string(name1) + "=" + FormatValue(t1);
  }

  template<typename T1, typename T2>
  std::string ArgumentList(
    const char* name1, const T1& t1
    , const char* name2, const T2& t2
  )
  {
    std::string FormatValue(const T1&);
    std::string FormatValue(const T2&);
    return std::string(name1) + "=" + FormatValue(t1)
      + ", " + name2 + "=" + FormatValue(t2);
  }

  template<typename T1, typename T2, typename T3>
  std::string ArgumentList(
    const char* name1, const T1& t1
    , const char* name2, const T2& t2
    , const char* name3, const T3& t3
  )
  {
    std::string FormatValue(const T1&);
    std::string FormatValue(const T2&);
    std::string FormatValue(const T3&);

    return std::string(name1) + "=" + FormatValue(t1)
      + ", " + name2 + "=" + FormatValue(t2)
      + ", " + name3 + "=" + FormatValue(t3);
  }

  template<typename T1, typename T2, typename T3, typename T4>
  std::string ArgumentList(
    const char* name1, const T1& t1
    , const char* name2, const T2& t2
    , const char* name3, const T3& t3
    , const char* name4, const T4& t4
  )
  {
    std::string FormatValue(const T1&);
    std::string FormatValue(const T2&);
    std::string FormatValue(const T3&);
    std::string FormatValue(const T4&);

    return std::string(name1) + "=" + FormatValue(t1)
      + ", " + name2 + "=" + FormatValue(t2)
      + ", " + name3 + "=" + FormatValue(t3)
      + ", " + name4 + "=" + FormatValue(t4);
  }

  template<typename T1, typename T2, typename T3, typename T4, typename T5>
  std::string ArgumentList(
    const char* name1, const T1& t1
    , const char* name2, const T2& t2
    , const char* name3, const T3& t3
    , const char* name4, const T4& t4
    , const char* name5, const T5& t5
  )
  {
    std::string FormatValue(const T1&);
    std::string FormatValue(const T2&);
    std::string FormatValue(const T3&);
    std::string FormatValue(const T4&);
    std::string FormatValue(const T5&);

    return std::string(name1) + "=" + FormatValue(t1)
      + ", " + name2 + "=" + FormatValue(t2)
      + ", " + name3 + "=" + FormatValue(t3)
      + ", " + name4 + "=" + FormatValue(t4)
      + ", " + name5 + "=" + FormatValue(t5);
  }
}

#define _ARGS1(arg1) \
  Logme::ArgumentList(#arg1, arg1).c_str()

#define _ARGS2(arg1, arg2) \
  Logme::ArgumentList(#arg1, arg1, #arg2, arg2).c_str()

#define _ARGS3(arg1, arg2, arg3) \
  Logme::ArgumentList(#arg1, arg1, #arg2, arg2, #arg3, arg3).c_str()

#define _ARGS4(arg1, arg2, arg3, arg4) \
  Logme::ArgumentList(#arg1, arg1, #arg2, arg2, #arg3, arg3, #arg4, arg4).c_str()

#define _ARGS5(arg1, arg2, arg3, arg4, arg5) \
  Logme::ArgumentList(#arg1, arg1, #arg2, arg2, #arg3, arg3, #arg4, arg4, #arg5, arg5).c_str()
