add_executable(Multithread
  Multithread.cpp
)

set(logme_target logme)
if(DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED)
  set(logme_target logmed)
endif()

if(NOT TARGET ${logme_target})
  message(FATAL_ERROR "Requested ${logme_target} but it is not built. Check LOGME_BUILD_STATIC/LOGME_BUILD_DYNAMIC and USE_LOGME_SHARED.")
endif()

 target_compile_options(Multithread PRIVATE /wd4840)

target_link_libraries(Multithread PRIVATE ${logme_target})



if(WIN32 AND DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED AND TARGET logmed)
  add_custom_command(TARGET Multithread POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:logmed>
      $<TARGET_FILE_DIR:Multithread>
  )
endif()

target_compile_definitions(Multithread PRIVATE
  LOGME_INRELEASE
)

if(NOT (DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED))
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
