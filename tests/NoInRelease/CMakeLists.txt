project(LogmeNoInReleaseTest)

add_executable(LogmeNoInReleaseTest
  NoInReleaseBuild.cpp
)

target_link_libraries(LogmeNoInReleaseTest PRIVATE logme)


if(WIN32 AND DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED)
  add_custom_command(TARGET LogmeNoInReleaseTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:logmed>
      $<TARGET_FILE_DIR:LogmeNoInReleaseTest>
  )
endif()
target_compile_definitions(LogmeNoInReleaseTest PRIVATE
  _LOGME_STATIC_BUILD_
)

if (WIN32)
  target_link_libraries(LogmeNoInReleaseTest PRIVATE ws2_32)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Tests")