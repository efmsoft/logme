add_executable(Multithread
  Multithread.cpp
)

target_link_libraries(Multithread PRIVATE ${LOGME_LINK_TARGET})
LogmeCopyRuntime(Multithread)

target_compile_definitions(Multithread PRIVATE
  LOGME_INRELEASE
)

if(LOGME_LINK_TARGET STREQUAL "logme")
  target_compile_definitions(Multithread PRIVATE
    _LOGME_STATIC_BUILD_
  )
endif()

if (WIN32)
  target_link_libraries(Multithread PRIVATE ws2_32)
endif()

set_target_properties(Multithread PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(Multithread PROPERTIES FOLDER "Examples")
