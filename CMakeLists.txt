cmake_minimum_required(VERSION 3.12)
project(logme VERSION 1.6.0)

cmake_policy(SET CMP0067 NEW)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 

option(LOGME_BUILD_EXAMPLES "Build logme examples" ON)
option(LOGME_BUILD_TESTS "Build logme tests" ON)
option(LOGME_BUILD_STATIC "Build logme as static library" ON)
option(LOGME_BUILD_DYNAMIC "Build logme as dynamic library" ON)
option(LOGME_BUILD_TOOLS "Build logme tools" ON)

set(LOGME_STD_FORMAT "AUTO" CACHE STRING "std::format logging API: AUTO, ON, or OFF")
set_property(CACHE LOGME_STD_FORMAT PROPERTY STRINGS AUTO ON OFF)

get_property(_isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (NOT _isMultiConfig)
  if (CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
  endif()
endif()

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


if(LOGME_BUILD_STATIC)
 add_subdirectory(logme ../out/Static)
endif()

if(LOGME_BUILD_DYNAMIC)
  add_subdirectory(dynamic ../out/Dynamic)
endif()

# Select which library target examples/tools/tests should link against.
# The selection is controlled by USE_LOGME_SHARED if it is defined by the root
# project or by a parent that calls add_subdirectory().
if(NOT TARGET logme::logme)
  if(DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED)
    if(TARGET logmed)
      add_library(logme::logme ALIAS logmed)
    else()
      message(FATAL_ERROR "USE_LOGME_SHARED is ON, but target 'logmed' is not available. Enable LOGME_BUILD_DYNAMIC.")
    endif()
  else()
    if(TARGET logme)
      add_library(logme::logme ALIAS logme)
    else()
      message(FATAL_ERROR "Target 'logme' is not available. Enable LOGME_BUILD_STATIC or set USE_LOGME_SHARED=ON with LOGME_BUILD_DYNAMIC.")
    endif()
  endif()
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install headers
install(
  DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/logme/include/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

# Install libraries (whatever was built)
set(_logme_install_targets "")
if(TARGET logme)
  list(APPEND _logme_install_targets logme)
endif()
if(TARGET logmed)
  list(APPEND _logme_install_targets logmed)
endif()

if(_logme_install_targets)
  install(
    TARGETS ${_logme_install_targets}
    EXPORT logmeTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  )

  install(
    EXPORT logmeTargets
    NAMESPACE logme::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/logme"
  )

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/logmeConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/logmeConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/logme"
  )

  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/logmeConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
  )

  install(
    FILES
      "${CMAKE_CURRENT_BINARY_DIR}/logmeConfig.cmake"
      "${CMAKE_CURRENT_BINARY_DIR}/logmeConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/logme"
  )
endif()

if(LOGME_BUILD_EXAMPLES)
  add_subdirectory(examples ../out/Examples)
endif()

if(LOGME_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests ../out/Tests)
endif()

if(LOGME_BUILD_TOOLS)
  add_subdirectory(tools ../out/Tools)
endif()
