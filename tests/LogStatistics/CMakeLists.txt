project(LogStatistics)

add_executable(${PROJECT_NAME}
  LogStatistics.cpp
  LogStatisticsC.c
)

if(LOGME_LINK_TARGET STREQUAL "logme")
  target_compile_definitions(${PROJECT_NAME} PRIVATE _LOGME_STATIC_BUILD_)
endif()

if(WIN32)
  set(WINDOWS_LIBRARIES Ws2_32.lib)
endif()

target_link_libraries(${PROJECT_NAME} ${LOGME_LINK_TARGET} gtest_main ${WINDOWS_LIBRARIES})
LogmeCopyRuntime(${PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME} PRIVATE LOGME_INRELEASE)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Tests")
