project(LevelFilter)

add_executable(${PROJECT_NAME} LevelFilter.cpp) 

if(WIN32)
  set(WINDOWS_LIBRARIES Ws2_32.lib)
endif()

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC logme gtest_main ${WINDOWS_LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PRIVATE _LOGME_STATIC_BUILD_)
target_compile_definitions(${PROJECT_NAME} PRIVATE LOGME_INRELEASE)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Tests")