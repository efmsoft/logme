project(LogmeNoInReleaseTest)

add_executable(LogmeNoInReleaseTest
  NoInReleaseBuild.cpp
)

set(logme_target logme)
if(DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED)
  set(logme_target logmed)
endif()

if(NOT TARGET ${logme_target})
  message(FATAL_ERROR "Requested ${logme_target} but it is not built. Check LOGME_BUILD_STATIC/LOGME_BUILD_DYNAMIC and USE_LOGME_SHARED.")
endif()


target_link_libraries(LogmeNoInReleaseTest PRIVATE ${logme_target})



if(WIN32 AND DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED AND TARGET logmed)
  add_custom_command(TARGET LogmeNoInReleaseTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:logmed>
      $<TARGET_FILE_DIR:LogmeNoInReleaseTest>
  )
endif()

if(NOT (DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED))
  target_compile_definitions(LogmeNoInReleaseTest PRIVATE
    _LOGME_STATIC_BUILD_
  )
endif()

if (WIN32)
  target_link_libraries(LogmeNoInReleaseTest PRIVATE ws2_32)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Tests")
