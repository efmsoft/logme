add_executable(ChannelSetup
  ChannelSetup.cpp
)

target_link_libraries(ChannelSetup PRIVATE logme)


if(WIN32 AND DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED)
  add_custom_command(TARGET ChannelSetup POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:logmed>
      $<TARGET_FILE_DIR:ChannelSetup>
  )
endif()
target_compile_definitions(ChannelSetup PRIVATE
  LOGME_INRELEASE
  _LOGME_STATIC_BUILD_
)

if (WIN32)
  target_link_libraries(ChannelSetup PRIVATE ws2_32)
endif()

set_target_properties(ChannelSetup PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(ChannelSetup PROPERTIES FOLDER "Examples")