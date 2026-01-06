cmake_minimum_required(VERSION 3.12)
project(logme VERSION 1.1.0)

cmake_policy(SET CMP0067 NEW)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 

if (NOT DEFINED LOGME_STD_FORMAT)
  set(LOGME_STD_FORMAT "AUTO" CACHE STRING "std::format logging API: AUTO, ON, or OFF")
endif()
set_property(CACHE LOGME_STD_FORMAT PROPERTY STRINGS AUTO ON OFF)

if (CMAKE_BUILD_TYPE STREQUAL "")
  message(STATUS "  Diag: Build type was unspecified, set to Release")
  set(CMAKE_BUILD_TYPE Release)
else ()
  message(STATUS "  Diag: Build type specified as '${CMAKE_BUILD_TYPE}'")
endif ()

if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
    set_directory_properties(PROPERTIES COMPILE_DEFINITIONS "_DEBUG")
endif ()

include(CheckCXXSourceCompiles)

set(_LOGME_STD_FORMAT_TEST [=[
#include <format>
#include <string>
#include <utility>

int main()
{
  auto s = std::format("{}", 1);
  (void)s;
  return 0;
}
]=])

string(TOUPPER "${LOGME_STD_FORMAT}" _LOGME_STD_FORMAT_MODE)
if (_LOGME_STD_FORMAT_MODE STREQUAL "OFF")
  set(LOGME_DISABLE_STD_FORMAT 1)
else()
  check_cxx_source_compiles("${_LOGME_STD_FORMAT_TEST}" LOGME_HAS_STD_FORMAT)
  if (LOGME_HAS_STD_FORMAT)
    set(LOGME_DISABLE_STD_FORMAT 0)
  else()
    if (_LOGME_STD_FORMAT_MODE STREQUAL "ON")
      message(FATAL_ERROR "LOGME_STD_FORMAT=ON but <format>/std::format is not available in this toolchain. Use GCC 13+ or Clang with libc++.")
    endif()
    set(LOGME_DISABLE_STD_FORMAT 1)
  endif()
endif()

set(LOGME_DISABLE_STD_FORMAT ${LOGME_DISABLE_STD_FORMAT} CACHE INTERNAL "Resolved std::format availability" FORCE)
if (LOGME_DISABLE_STD_FORMAT)
  add_compile_definitions(LOGME_DISABLE_STD_FORMAT)
endif()

enable_testing()

include_directories(${CMAKE_SOURCE_DIR}/logme/include)

add_subdirectory(logme ../out/Static)
add_subdirectory(dynamic ../out/Dynamic)
option(LOGME_BUILD_EXAMPLES "Build logme examples" ON)

if(LOGME_BUILD_EXAMPLES)
  add_subdirectory(examples ../out/Examples)
endif()

add_subdirectory(tests ../out/Tests)

