add_executable(FormatApi
  FormatApi.cpp
)

target_link_libraries(FormatApi PRIVATE ${LOGME_LINK_TARGET})
LogmeCopyRuntime(FormatApi)

target_compile_definitions(FormatApi PRIVATE
  LOGME_INRELEASE
)

if(LOGME_LINK_TARGET STREQUAL "logme")
  target_compile_definitions(FormatApi PRIVATE
    _LOGME_STATIC_BUILD_
  )
endif()

if (WIN32)
  target_link_libraries(FormatApi PRIVATE ws2_32)
endif()

set_target_properties(FormatApi PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(FormatApi PROPERTIES FOLDER "Examples")
