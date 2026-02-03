project(LogmeNoInReleaseTest)

add_executable(LogmeNoInReleaseTest
  NoInReleaseBuild.cpp
)

target_link_libraries(LogmeNoInReleaseTest PRIVATE ${LOGME_LINK_TARGET})
LogmeCopyRuntime(LogmeNoInReleaseTest)

if(LOGME_LINK_TARGET STREQUAL "logme")
  target_compile_definitions(LogmeNoInReleaseTest PRIVATE
    _LOGME_STATIC_BUILD_
  )
endif()

if (WIN32)
  target_link_libraries(LogmeNoInReleaseTest PRIVATE ws2_32)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Tests")
