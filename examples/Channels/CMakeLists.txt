add_executable(Channels
  Channels.cpp
)

target_link_libraries(Channels PRIVATE logme)


if(WIN32 AND DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED)
  add_custom_command(TARGET Channels POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:logmed>
      $<TARGET_FILE_DIR:Channels>
  )
endif()
target_compile_definitions(Channels PRIVATE
  LOGME_INRELEASE
  _LOGME_STATIC_BUILD_
)

if (WIN32)
  target_link_libraries(Channels PRIVATE ws2_32)
endif()

set_target_properties(Channels PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(Channels PROPERTIES FOLDER "Examples")