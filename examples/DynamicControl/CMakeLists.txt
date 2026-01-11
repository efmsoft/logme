add_executable(DynamicControl
  main.cpp
  Worker.h
  Worker.cpp
)

set_target_properties(DynamicControl PROPERTIES
  FOLDER "Examples"
)

target_link_libraries(DynamicControl PRIVATE logme)
target_compile_definitions(DynamicControl PRIVATE
  LOGME_INRELEASE
  _LOGME_STATIC_BUILD_
)

if (WIN32)
  target_link_libraries(DynamicControl PRIVATE Ws2_32)
endif()


# Force link all objects from logme static library to keep command registrars in static builds.
if (MSVC)
  target_link_options(DynamicControl PRIVATE "/WHOLEARCHIVE:$<TARGET_LINKER_FILE:logme>")
elseif(UNIX AND NOT APPLE)
  target_link_options(DynamicControl PRIVATE
    "-Wl,--whole-archive" "$<TARGET_LINKER_FILE:logme>" "-Wl,--no-whole-archive"
  )
endif()

set_target_properties(DynamicControl PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

