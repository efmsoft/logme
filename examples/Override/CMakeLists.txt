add_executable(Override
  Override.cpp
)

target_link_libraries(Override PRIVATE ${LOGME_LINK_TARGET})
LogmeCopyRuntime(Override)

target_compile_definitions(Override PRIVATE
  LOGME_INRELEASE
)

if(LOGME_LINK_TARGET STREQUAL "logme")
  target_compile_definitions(Override PRIVATE
    _LOGME_STATIC_BUILD_
  )
endif()

if (WIN32)
  target_link_libraries(Override PRIVATE ws2_32)
endif()

set_target_properties(Override PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(Override PROPERTIES FOLDER "Examples")
