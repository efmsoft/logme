add_executable(ExampleTracePoints
  TracePoints.cpp
)

target_link_libraries(ExampleTracePoints PRIVATE ${LOGME_LINK_TARGET})
LogmeCopyRuntime(ExampleTracePoints)

target_compile_definitions(ExampleTracePoints PRIVATE
  LOGME_INRELEASE
)

if(LOGME_LINK_TARGET STREQUAL "logme")
  target_compile_definitions(ExampleTracePoints PRIVATE
    _LOGME_STATIC_BUILD_
  )
endif()

if (WIN32)
  target_link_libraries(ExampleTracePoints PRIVATE ws2_32)
endif()

set_target_properties(ExampleTracePoints PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(ExampleTracePoints PROPERTIES FOLDER "Examples")
