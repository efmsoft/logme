project(Tools)
add_executable(${PROJECT_NAME} Tools.cpp)

if(WIN32)
  set(WINDOWS_LIBRARIES Ws2_32.lib)
endif()

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC gtest_main ${WINDOWS_LIBRARIES})

add_dependencies(${PROJECT_NAME} logmefmt logmeobf)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  LOGMEFMT_EXE="$<TARGET_FILE:logmefmt>"
  LOGMEOBF_EXE="$<TARGET_FILE:logmeobf>"
)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}
  PROPERTIES RUN_SERIAL TRUE
)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Tests")
