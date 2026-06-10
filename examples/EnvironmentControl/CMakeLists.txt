add_executable(EnvironmentControl
  EnvironmentControl.cpp
)

target_link_libraries(EnvironmentControl PRIVATE ${LOGME_LINK_TARGET})
LogmeCopyRuntime(EnvironmentControl)

target_compile_definitions(EnvironmentControl PRIVATE
  LOGME_INRELEASE
)

if(LOGME_LINK_TARGET STREQUAL "logme")
  target_compile_definitions(EnvironmentControl PRIVATE
    _LOGME_STATIC_BUILD_
  )
endif()

if (WIN32)
  target_link_libraries(EnvironmentControl PRIVATE ws2_32)
endif()

set_target_properties(EnvironmentControl PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(EnvironmentControl PROPERTIES FOLDER "Examples")
