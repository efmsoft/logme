add_executable(FormatApi
  FormatApi.cpp
)

target_link_libraries(FormatApi PRIVATE logme)

target_compile_definitions(FormatApi PRIVATE
  LOGME_INRELEASE
  _LOGME_STATIC_BUILD_
)

if (WIN32)
  target_link_libraries(FormatApi PRIVATE ws2_32)
endif()

set_target_properties(FormatApi PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(FormatApi PROPERTIES FOLDER "Examples")