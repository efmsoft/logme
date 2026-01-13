project(ChannelRedirect)

add_executable(${PROJECT_NAME} ChannelRedirect.cpp) 

if(WIN32)
  set(WINDOWS_LIBRARIES Ws2_32.lib)
endif()

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC logme gtest_main ${WINDOWS_LIBRARIES})

if(WIN32 AND DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:logmed>
      $<TARGET_FILE_DIR:${PROJECT_NAME}>
  )
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE _LOGME_STATIC_BUILD_)
target_compile_definitions(${PROJECT_NAME} PRIVATE LOGME_INRELEASE)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Tests")