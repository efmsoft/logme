add_executable(Basic
  Basic.cpp
)

target_link_libraries(Basic PRIVATE logme)

target_compile_definitions(Basic PRIVATE
  LOGME_INRELEASE
  _LOGME_STATIC_BUILD_
)

if (WIN32)
  target_link_libraries(Basic PRIVATE ws2_32)
endif()

set_target_properties(Basic PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(Basic PROPERTIES FOLDER "Examples")