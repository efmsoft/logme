add_executable(DumpBuffer
  DumpBuffer.cpp
)

target_link_libraries(DumpBuffer PRIVATE ${LOGME_LINK_TARGET})
LogmeCopyRuntime(DumpBuffer)

target_compile_definitions(DumpBuffer PRIVATE
  LOGME_INRELEASE
)

if(LOGME_LINK_TARGET STREQUAL "logme")
  target_compile_definitions(DumpBuffer PRIVATE
    _LOGME_STATIC_BUILD_
  )
endif()

if (WIN32)
  target_link_libraries(DumpBuffer PRIVATE ws2_32)
endif()

set_target_properties(DumpBuffer PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(DumpBuffer PROPERTIES FOLDER "Examples")
