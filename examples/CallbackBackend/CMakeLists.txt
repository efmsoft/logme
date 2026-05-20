add_executable(ExampleCallbackBackend
  CallbackBackend.cpp
)

target_link_libraries(ExampleCallbackBackend PRIVATE ${LOGME_LINK_TARGET})
LogmeCopyRuntime(ExampleCallbackBackend)

target_compile_definitions(ExampleCallbackBackend PRIVATE
  LOGME_INRELEASE
)

if(LOGME_LINK_TARGET STREQUAL "logme")
  target_compile_definitions(ExampleCallbackBackend PRIVATE
    _LOGME_STATIC_BUILD_
  )
endif()

if (WIN32)
  target_link_libraries(ExampleCallbackBackend PRIVATE ws2_32)
endif()

set_target_properties(ExampleCallbackBackend PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(ExampleCallbackBackend PROPERTIES FOLDER "Examples")
