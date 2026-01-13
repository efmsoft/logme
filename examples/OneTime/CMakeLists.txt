project(OneTime)

add_executable(${PROJECT_NAME} OneTime.cpp)

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC logme)


if(WIN32 AND DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:logmed>
      $<TARGET_FILE_DIR:${PROJECT_NAME}>
  )
endif()
if (WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE _LOGME_STATIC_BUILD_ LOGME_INRELEASE)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Examples")

set_target_properties(${PROJECT_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)
