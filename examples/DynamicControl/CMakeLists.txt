add_executable(DynamicControl
  main.cpp
  Worker.h
  Worker.cpp
)

set(logme_target logme)
if(DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED)
  set(logme_target logmed)
endif()

if(NOT TARGET ${logme_target})
  message(FATAL_ERROR "Requested ${logme_target} but it is not built. Check LOGME_BUILD_STATIC/LOGME_BUILD_DYNAMIC and USE_LOGME_SHARED.")
endif()


set_target_properties(DynamicControl PROPERTIES
  FOLDER "Examples"
)

target_link_libraries(DynamicControl PRIVATE ${logme_target})


if(WIN32 AND DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED AND TARGET logmed)
  add_custom_command(TARGET DynamicControl POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:logmed>
      $<TARGET_FILE_DIR:DynamicControl>
  )
endif()

target_compile_definitions(DynamicControl PRIVATE
  LOGME_INRELEASE
)

if(NOT (DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED))
  target_compile_definitions(DynamicControl PRIVATE
    _LOGME_STATIC_BUILD_
  )
endif()

if (WIN32)
  target_link_libraries(DynamicControl PRIVATE Ws2_32)
endif()


# Force link all objects from logme static library to keep command registrars in static builds.
if(NOT (DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED))
  if (MSVC)
    target_link_options(DynamicControl PRIVATE "/WHOLEARCHIVE:$<TARGET_LINKER_FILE:logme>")
  elseif(UNIX AND NOT APPLE)
    target_link_options(DynamicControl PRIVATE
      "-Wl,--whole-archive" "$<TARGET_LINKER_FILE:logme>" "-Wl,--no-whole-archive"
    )
  endif()
endif()


set_target_properties(DynamicControl PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)
