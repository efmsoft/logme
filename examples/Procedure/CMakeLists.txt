add_executable(ExampleProcedure
  Procedure.cpp
)

target_link_libraries(ExampleProcedure PRIVATE ${LOGME_LINK_TARGET})
LogmeCopyRuntime(ExampleProcedure)

target_compile_definitions(ExampleProcedure PRIVATE
  LOGME_INRELEASE
)

if(LOGME_LINK_TARGET STREQUAL "logme")
  target_compile_definitions(ExampleProcedure PRIVATE
    _LOGME_STATIC_BUILD_
  )
endif()

if (WIN32)
  target_link_libraries(ExampleProcedure PRIVATE ws2_32)
endif()

set_target_properties(ExampleProcedure PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(ExampleProcedure PROPERTIES FOLDER "Examples")
