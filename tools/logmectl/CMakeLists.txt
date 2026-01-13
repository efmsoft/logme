add_executable(logmectl
  main.cpp
)

set_target_properties(logmectl PROPERTIES
  FOLDER "Tools"
)

target_link_libraries(logmectl PRIVATE logme)


if(WIN32 AND DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED)
  add_custom_command(TARGET logmectl POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:logmed>
      $<TARGET_FILE_DIR:logmectl>
  )
endif()
if (WIN32)
  target_link_libraries(logmectl PRIVATE Ws2_32)
endif()

if (UNIX AND NOT APPLE)
  target_link_libraries(logmectl PRIVATE dl)
endif()
