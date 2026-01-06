add_executable(ExampleProcedure
  Procedure.cpp
)

target_link_libraries(ExampleProcedure PRIVATE logme)

target_compile_definitions(ExampleProcedure PRIVATE
  LOGME_INRELEASE
)

set_target_properties(ExampleProcedure PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)
