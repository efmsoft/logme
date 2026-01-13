add_executable(Override
  Override.cpp
)

target_link_libraries(Override PRIVATE logme)


if(WIN32 AND DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED)
  add_custom_command(TARGET Override POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:logmed>
      $<TARGET_FILE_DIR:Override>
  )
endif()
target_compile_definitions(Override PRIVATE
  LOGME_INRELEASE
  _LOGME_STATIC_BUILD_
)

if (WIN32)
  target_link_libraries(Override PRIVATE ws2_32)
endif()

set_target_properties(Override PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(Override PROPERTIES FOLDER "Examples")