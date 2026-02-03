add_executable(ChannelSetup
  ChannelSetup.cpp
)

target_link_libraries(ChannelSetup PRIVATE ${LOGME_LINK_TARGET})
LogmeCopyRuntime(ChannelSetup)

target_compile_definitions(ChannelSetup PRIVATE
  LOGME_INRELEASE
)

if(LOGME_LINK_TARGET STREQUAL "logme")
  target_compile_definitions(ChannelSetup PRIVATE
    _LOGME_STATIC_BUILD_
  )
endif()

if (WIN32)
  target_link_libraries(ChannelSetup PRIVATE ws2_32)
endif()

set_target_properties(ChannelSetup PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(ChannelSetup PROPERTIES FOLDER "Examples")
