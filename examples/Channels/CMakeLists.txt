add_executable(Channels
  Channels.cpp
)

target_link_libraries(Channels PRIVATE logme)

target_compile_definitions(Channels PRIVATE
  LOGME_INRELEASE
  _LOGME_STATIC_BUILD_
)

if (WIN32)
  target_link_libraries(Channels PRIVATE ws2_32)
endif()

set_target_properties(Channels PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(Channels PROPERTIES FOLDER "Examples")