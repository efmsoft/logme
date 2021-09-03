add_executable(LevelFilter LevelFilter.cpp) 
target_link_libraries(LevelFilter LINK_PUBLIC logme gtest_main) 

include(GoogleTest)
gtest_discover_tests(LevelFilter)