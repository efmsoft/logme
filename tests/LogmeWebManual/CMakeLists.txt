project(LogmeWebManual)

add_executable(LogmeWebManual
  main.cpp
)

set_target_properties(LogmeWebManual PROPERTIES
  FOLDER "Tests"
)

target_link_libraries(LogmeWebManual PRIVATE ${LOGME_LINK_TARGET})
LogmeCopyRuntime(LogmeWebManual)

target_compile_definitions(LogmeWebManual PRIVATE
  LOGME_INRELEASE
)

if(LOGME_LINK_TARGET STREQUAL "logme")
  target_compile_definitions(LogmeWebManual PRIVATE
    _LOGME_STATIC_BUILD_
  )
endif()

if(WIN32)
  target_link_libraries(LogmeWebManual PRIVATE Ws2_32)

  if(NOT DEFINED VCPKG_TARGET_TRIPLET OR VCPKG_TARGET_TRIPLET STREQUAL "")
    set(_logmeweb_vcpkg_triplet "x64-windows")
  else()
    set(_logmeweb_vcpkg_triplet "${VCPKG_TARGET_TRIPLET}")
  endif()

  set(_logmeweb_vcpkg_roots)

  if(DEFINED CMAKE_TOOLCHAIN_FILE AND NOT CMAKE_TOOLCHAIN_FILE STREQUAL "")
    get_filename_component(_logmeweb_toolchain_file "${CMAKE_TOOLCHAIN_FILE}" ABSOLUTE)
    get_filename_component(_logmeweb_buildsystems_dir "${_logmeweb_toolchain_file}" DIRECTORY)
    get_filename_component(_logmeweb_scripts_dir "${_logmeweb_buildsystems_dir}" DIRECTORY)
    get_filename_component(_logmeweb_vcpkg_root_from_toolchain "${_logmeweb_scripts_dir}" DIRECTORY)
    if(EXISTS "${_logmeweb_vcpkg_root_from_toolchain}/installed")
      list(APPEND _logmeweb_vcpkg_roots "${_logmeweb_vcpkg_root_from_toolchain}")
    endif()
  endif()

  if(DEFINED VCPKG_ROOT AND NOT VCPKG_ROOT STREQUAL "")
    list(APPEND _logmeweb_vcpkg_roots "${VCPKG_ROOT}")
  endif()

  if(DEFINED ENV{VCPKG_ROOT} AND NOT "$ENV{VCPKG_ROOT}" STREQUAL "")
    list(APPEND _logmeweb_vcpkg_roots "$ENV{VCPKG_ROOT}")
  endif()

  list(APPEND _logmeweb_vcpkg_roots "C:/dev/vcpkg")
  list(REMOVE_DUPLICATES _logmeweb_vcpkg_roots)

  set(_logmeweb_openssl_runtime_dirs
    "${CMAKE_BINARY_DIR}/vcpkg_installed/${_logmeweb_vcpkg_triplet}/bin"
    "${CMAKE_BINARY_DIR}/vcpkg_installed/${_logmeweb_vcpkg_triplet}/debug/bin"
  )

  foreach(_logmeweb_vcpkg_root IN LISTS _logmeweb_vcpkg_roots)
    list(APPEND _logmeweb_openssl_runtime_dirs
      "${_logmeweb_vcpkg_root}/installed/${_logmeweb_vcpkg_triplet}/bin"
      "${_logmeweb_vcpkg_root}/installed/${_logmeweb_vcpkg_triplet}/debug/bin"
    )
  endforeach()

  list(REMOVE_DUPLICATES _logmeweb_openssl_runtime_dirs)

  set(_logmeweb_openssl_dlls)
  foreach(_logmeweb_openssl_dir IN LISTS _logmeweb_openssl_runtime_dirs)
    if(EXISTS "${_logmeweb_openssl_dir}")
      file(GLOB _logmeweb_openssl_dir_dlls
        "${_logmeweb_openssl_dir}/libssl*.dll"
        "${_logmeweb_openssl_dir}/libcrypto*.dll"
      )
      list(APPEND _logmeweb_openssl_dlls ${_logmeweb_openssl_dir_dlls})
    endif()
  endforeach()

  if(_logmeweb_openssl_dlls)
    list(REMOVE_DUPLICATES _logmeweb_openssl_dlls)
    message(STATUS "LogmeWebManual OpenSSL runtime DLLs: ${_logmeweb_openssl_dlls}")
    foreach(_logmeweb_openssl_dll IN LISTS _logmeweb_openssl_dlls)
      add_custom_command(TARGET LogmeWebManual POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${_logmeweb_openssl_dll}"
          $<TARGET_FILE_DIR:LogmeWebManual>
      )
    endforeach()
  else()
    message(STATUS "LogmeWebManual OpenSSL runtime DLLs were not found during configure")
  endif()
endif()

# Force link all objects from logme static library to keep command registrars in static builds.
if(LOGME_LINK_TARGET STREQUAL "logme")
  if(MSVC)
    target_link_options(LogmeWebManual PRIVATE "/WHOLEARCHIVE:$<TARGET_LINKER_FILE:logme>")
  elseif(UNIX AND NOT APPLE)
    target_link_options(LogmeWebManual PRIVATE
      "-Wl,--whole-archive" "$<TARGET_LINKER_FILE:logme>" "-Wl,--no-whole-archive"
    )
  endif()
endif()

set_target_properties(LogmeWebManual PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Tests"
)
