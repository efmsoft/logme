project(OutputFlags)
add_executable(${PROJECT_NAME} OutputFlags.cpp) 

if(WIN32)
  set(WINDOWS_LIBRARIES Ws2_32.lib)
endif()

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC ${LOGME_LINK_TARGET} gtest_main ${WINDOWS_LIBRARIES}) 
LogmeCopyRuntime(${PROJECT_NAME})

if(LOGME_LINK_TARGET STREQUAL "logme")
  target_compile_definitions(${PROJECT_NAME} PRIVATE
    _LOGME_STATIC_BUILD_
  )
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE LOGME_INRELEASE)


if(WIN32 AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.21")
  # Ensure all runtime DLL dependencies are available before test discovery.
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
      $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND_EXPAND_LISTS
  )
endif()
include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Tests")
