add_executable(LogStatisticsProfiling
  LogStatisticsProfiling.cpp
)

set_target_properties(LogStatisticsProfiling PROPERTIES
  FOLDER "Examples"
)

target_link_libraries(LogStatisticsProfiling PRIVATE ${LOGME_LINK_TARGET})

LogmeCopyRuntime(LogStatisticsProfiling)

target_compile_definitions(LogStatisticsProfiling PRIVATE
  LOGME_INRELEASE
)

if(LOGME_LINK_TARGET STREQUAL "logme")
  target_compile_definitions(LogStatisticsProfiling PRIVATE
    _LOGME_STATIC_BUILD_
  )
endif()

if (WIN32)
  target_link_libraries(LogStatisticsProfiling PRIVATE Ws2_32)
endif()

# Force link all objects from logme static library to keep command registrars in static builds.
if(LOGME_LINK_TARGET STREQUAL "logme")
  if (MSVC)
    target_link_options(LogStatisticsProfiling PRIVATE "/WHOLEARCHIVE:$<TARGET_LINKER_FILE:logme>")
  elseif(UNIX AND NOT APPLE)
    target_link_options(LogStatisticsProfiling PRIVATE
      "-Wl,--whole-archive" "$<TARGET_LINKER_FILE:logme>" "-Wl,--no-whole-archive"
    )
  endif()
endif()

set_target_properties(LogStatisticsProfiling PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)
