add_executable(StructuredOutput
  StructuredOutput.cpp
)

target_link_libraries(StructuredOutput PRIVATE ${LOGME_LINK_TARGET})
LogmeCopyRuntime(StructuredOutput)

target_compile_definitions(StructuredOutput PRIVATE
  LOGME_INRELEASE
)

if(LOGME_LINK_TARGET STREQUAL "logme")
  target_compile_definitions(StructuredOutput PRIVATE
    _LOGME_STATIC_BUILD_
  )
endif()

if (WIN32)
  target_link_libraries(StructuredOutput PRIVATE ws2_32)
endif()

set_target_properties(StructuredOutput PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(StructuredOutput PROPERTIES FOLDER "Examples")
