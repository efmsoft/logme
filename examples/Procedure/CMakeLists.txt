add_executable(ExampleProcedure
  Procedure.cpp
)

target_link_libraries(ExampleProcedure PRIVATE logme)

target_compile_definitions(ExampleProcedure PRIVATE
  LOGME_INRELEASE
  _LOGME_STATIC_BUILD_
)

if (WIN32)
  target_link_libraries(ExampleProcedure PRIVATE ws2_32)
endif()

set_target_properties(ExampleProcedure PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(ExampleProcedure PROPERTIES FOLDER "Examples")