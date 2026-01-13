add_executable(Channels
  Channels.cpp
)

set(logme_target logme)
if(DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED)
  set(logme_target logmed)
endif()

if(NOT TARGET ${logme_target})
  message(FATAL_ERROR "Requested ${logme_target} but it is not built. Check LOGME_BUILD_STATIC/LOGME_BUILD_DYNAMIC and USE_LOGME_SHARED.")
endif()


target_link_libraries(Channels PRIVATE ${logme_target})



if(WIN32 AND DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED AND TARGET logmed)
  add_custom_command(TARGET Channels POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:logmed>
      $<TARGET_FILE_DIR:Channels>
  )
endif()

target_compile_definitions(Channels PRIVATE
  LOGME_INRELEASE
)

if(NOT (DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED))
  target_compile_definitions(Channels PRIVATE
    _LOGME_STATIC_BUILD_
  )
endif()

if (WIN32)
  target_link_libraries(Channels PRIVATE ws2_32)
endif()

set_target_properties(Channels PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(Channels PROPERTIES FOLDER "Examples")
