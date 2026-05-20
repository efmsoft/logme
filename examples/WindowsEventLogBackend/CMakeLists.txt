if(NOT WIN32)
  return()
endif()

add_executable(ExampleWindowsEventLogBackend
  WindowsEventLogBackend.cpp
)

target_link_libraries(ExampleWindowsEventLogBackend PRIVATE
  ${LOGME_LINK_TARGET}
  ws2_32
)
LogmeCopyRuntime(ExampleWindowsEventLogBackend)

target_compile_definitions(ExampleWindowsEventLogBackend PRIVATE
  LOGME_INRELEASE
)

if(LOGME_LINK_TARGET STREQUAL "logme")
  target_compile_definitions(ExampleWindowsEventLogBackend PRIVATE
    _LOGME_STATIC_BUILD_
  )
endif()

set_target_properties(ExampleWindowsEventLogBackend PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(ExampleWindowsEventLogBackend PROPERTIES FOLDER "Examples")
