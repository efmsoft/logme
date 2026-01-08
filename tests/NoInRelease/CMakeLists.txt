project(LogmeNoInReleaseTest)

add_executable(LogmeNoInReleaseTest
  NoInReleaseBuild.cpp
)

target_link_libraries(LogmeNoInReleaseTest PRIVATE logme)

target_compile_definitions(LogmeNoInReleaseTest PRIVATE
  _LOGME_STATIC_BUILD_
)

if (WIN32)
  target_link_libraries(LogmeNoInReleaseTest PRIVATE ws2_32)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Tests")