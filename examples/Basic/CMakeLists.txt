add_executable(Basic
  Basic.cpp
)

target_link_libraries(Basic PRIVATE ${LOGME_LINK_TARGET})
LogmeCopyRuntime(Basic)

target_compile_definitions(Basic PRIVATE
  LOGME_INRELEASE
)

if(LOGME_LINK_TARGET STREQUAL "logme")
  target_compile_definitions(Basic PRIVATE
    _LOGME_STATIC_BUILD_
  )
endif()

if (WIN32)
  target_link_libraries(Basic PRIVATE ws2_32)
endif()

set_target_properties(Basic PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(Basic PROPERTIES FOLDER "Examples")
